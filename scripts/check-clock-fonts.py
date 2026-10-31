#!/usr/bin/env python3
"""Verify Dynamic's compiled clock/date/context metrics against its exact PBW."""
import argparse
import calendar
import json
from pathlib import Path
import re
import struct
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
CLOCK_CHARS="0123456789:"
RESOURCE_TABLE_BYTES=256*16

def require(condition, message):
    if not condition:
        raise ValueError(message)

def read_font(packed, resource_id, chars=CLOCK_CHARS, ink_bottoms=False):
    """Read glyph metrics from a compiled Pebble v3 font resource."""
    record = 12 + 16 * (resource_id - 1)
    require(resource_id > 0 and record + 16 <= len(packed), "Invalid font resource ID")
    file_id, offset, length, _crc = struct.unpack_from("<IIII", packed, record)
    require(file_id == resource_id, "Resource header/pack ID mismatch")
    start = 12 + RESOURCE_TABLE_BYTES + offset
    require(start + length <= len(packed), "Font resource extends beyond the resource pack")
    data = packed[start:start + length]
    require(len(data) >= 10, "Truncated compiled font")
    version, _height, count, wildcard, buckets, cp_bytes, header_size, features = struct.unpack_from(
        "<BBHHBBBB", data)
    require(version == 3, "Unsupported compiled font version; update the verifier")
    require(cp_bytes in (2, 4) and header_size >= 10, "Invalid compiled font header")
    require(features & ~1 == 0, "Unsupported compiled font encoding; raw bitmap glyphs required")
    offset_bytes = 2 if features & 1 else 4
    table = header_size + buckets * 4
    entries = {}
    total = 0
    for bucket in range(buckets):
        _hash, number, position = struct.unpack_from("<BBH", data, header_size + bucket * 4)
        total += number
        bucket_entries = []
        for index in range(number):
            at = table + position + index * (cp_bytes + offset_bytes)
            require(at + cp_bytes + offset_bytes <= len(data), "Truncated font lookup table")
            codepoint = int.from_bytes(data[at:at + cp_bytes], "little")
            glyph_offset = int.from_bytes(data[at + cp_bytes:at + cp_bytes + offset_bytes], "little")
            # The SDK writes a reserved fallback and a concrete wildcard
            # glyph under the same codepoint. Clock digits must stay unique.
            require(codepoint not in entries or codepoint == wildcard,
                    "Duplicate compiled font codepoint")
            bucket_entries.append((codepoint, glyph_offset))
            entries.setdefault(codepoint, glyph_offset)
        # Match the firmware's binary search exactly, including the SDK's
        # duplicate wildcard entries. In this repertoire it chooses the
        # reserved missing glyph rather than the later concrete square.
        require(bucket_entries == sorted(bucket_entries, key=lambda entry: entry[0]),
                "Compiled font bucket is not sorted by codepoint")
        for codepoint in {entry[0] for entry in bucket_entries}:
            left, right = 0, len(bucket_entries) - 1
            while left <= right:
                middle = (left + right) // 2
                candidate, glyph_offset = bucket_entries[middle]
                if candidate < codepoint:
                    left = middle + 1
                elif candidate > codepoint:
                    right = middle - 1
                else:
                    entries[codepoint] = glyph_offset
                    break
    require(total == count, "Compiled font glyph count differs from its lookup table")
    glyph_base = table + total * (cp_bytes + offset_bytes)
    metrics = {}
    all_glyphs = chars is None
    require(not all_glyphs or wildcard == 0x25af, "Context font wildcard codepoint changed")
    for char in (map(chr, sorted(entries)) if all_glyphs else chars):
        codepoint = ord(char)
        require(codepoint in entries and (all_glyphs or codepoint != wildcard),
                "Required glyph missing: " + char)
        at = glyph_base + entries[codepoint]
        require(at + 5 <= len(data), "Truncated glyph: " + char)
        width, height, left, top, advance = struct.unpack_from("<BBbbb", data, at)
        require(advance > 0 and (char == " " or (width > 0 and height > 0)),
                "Empty required glyph: " + char)
        if ink_bottoms:
            byte_count = (width * height + 7) // 8
            require(at + 5 + byte_count <= len(data), "Truncated glyph bitmap: " + char)
            bitmap = data[at + 5:at + 5 + byte_count]
            last = next((bit for bit in range(width * height - 1, -1, -1)
                         if bitmap[bit // 8] & (1 << (bit % 8))), None)
            metrics[char] = top + last // width + 1 if last is not None else 0
        else:
            metrics[char] = (width, height, left, top, advance)
    return metrics

def measure(text, metrics):
    advance = 0
    left = top = sys.maxsize
    right = bottom = -sys.maxsize
    for char in text:
        width, height, x, y, step = metrics[char]
        if width and height:
            left = min(left, advance + x)
            right = max(right, advance + x + width)
            top = min(top, y)
            bottom = max(bottom, y + height)
        advance += step
    return advance, left, right, top, bottom


def verify(pbw):
    package=json.loads((ROOT/'package.json').read_text())
    ids={name:int(number) for name,number in re.findall(
        r'#define\s+RESOURCE_ID_(\w+)\s+(\d+)',
        (ROOT/'build/emery/src/resource_ids.auto.h').read_text())}
    with zipfile.ZipFile(pbw) as z:
        require(z.testzip() is None,'Invalid PBW archive')
        info=json.loads(z.read('appinfo.json'))
        require(info['uuid']==package['pebble']['uuid'],'PBW identity mismatch')
        require(info['versionLabel']==package['version'],'PBW version mismatch')
        for name in ('pebble-app.bin','app_resources.pbpack'):
            require(z.read('emery/'+name)==(ROOT/'build/emery'/name).read_bytes(),
                    'PBW does not match the local build: '+name)
        packed=z.read('emery/app_resources.pbpack')
    layout=(ROOT/'src/c/watch_layout.h').read_text()
    rows=[tuple(map(int,row)) for row in re.findall(r'\{(\d+),(\d+),(\d+)\}',
        layout.split('fonts[] = {',1)[1].split('};',1)[0])]
    require(len(rows)==13,'Expected all13 clock font bounds')
    fonts={}
    for size,top,bottom in rows:
        name='FIRA_SANS_CONDENSED_BOLD_'+('COMPLICATION_' if size in (38,44) else '')+str(size)
        fonts[size]=read_font(packed,ids[name])
        actual=(min(g[3] for g in fonts[size].values()),max(g[3]+g[1] for g in fonts[size].values()))
        require((top,bottom)==actual,'Clock ink bound drift: '+str(size))
    require('contextual ? 56 :' in layout and 'watch_layout_interpolate(108,74,height)' in layout,
            'Clock center policy changed; review vertical budgets')
    bounds={n:(t,b) for n,t,b in rows}
    checks=0
    minimum_tops={}
    clocks=([f'{h:02d}:{m:02d}' for h in range(24) for m in range(60)]+
            [f'{h}:{m:02d}' for h in range(1,13) for m in range(60)])
    for text in clocks:
        for big in (False,True):
            for mode in ('full','context','peek'):
                for shadow in (False,True):
                    width=192
                    if mode=='context':size=44 if big else 38
                    elif mode=='peek':size=72 if big else 64
                    else:
                        size=next((n for n in (108,104,100,96,92,88,84,80)
                            if n<=(108 if big else 92) and measure(text,fonts[n])[0]<=width-(6 if shadow else 4)),72)
                    advance,left,right,top,bottom=measure(text,fonts[size])
                    x=(width-advance)//2
                    require(x+left>=0 and x+right+(1 if shadow else 0)<=width,'Clock width overflow: '+text)
                    center={'full':108,'context':56,'peek':74}[mode]
                    y=center-sum(bounds[size])//2
                    actual_top=y+top;actual_bottom=y+bottom+(1 if shadow else 0)
                    limit={'full':151,'context':76,'peek':110}[mode]
                    require(actual_bottom<=limit,'Clock/reading overlap: '+text+'/'+mode)
                    require(actual_top>={'full':44,'context':39,'peek':40}[mode],'Clock/date overlap: '+text+'/'+mode)
                    key=(big,mode,shadow);minimum_tops[key]=min(minimum_tops.get(key,999),actual_top)
                    checks+=1
    native=(ROOT/'src/c/dynamic.c').read_text()
    arrays=re.findall(r'static const char \*const s_(\w+)_days\[\]\s*=\s*\{(.*?)\};',native,re.S)
    require(len(arrays)==17,'Expected17 locale weekday arrays')
    words=[(language,day) for language,body in arrays for day in re.findall(r'"([^"]+)"',body)]
    chars=sorted(set('0123456789 ·.'+''.join(day for _,day in words)))
    dates={n:read_font(packed,ids['FIRA_SANS_CONDENSED_BOLD_'+str(n)],chars) for n in (28,24,22,20)}
    dates[18]=read_font(packed,ids['FIRA_SANS_CONDENSED_BOLD_COMPLICATION_18'],chars)
    strings=date_checks=0;minimum_gap=999;minimum_date_top=999
    for language,day in words:
        for month in range(1,13):
            for date in range(1,calendar.monthrange(2028,month)[1]+1):
                text=f'{day} · {date}.{month}.'
                ms={n:measure(text,font) for n,font in dates.items()};strings+=1
                for big in (False,True):
                    for mode in ('full','context','peek'):
                        for shadow in (False,True):
                            candidates=((28,24,22,20,18) if big else(24,22,20,18)) if mode=='full' else((22,20,18) if big else(20,22,20,18))
                            size=next((n for n in candidates if ms[n][0]<=188-(2 if shadow else 0)),18)
                            advance,left,right,top,bottom=ms[size]
                            x=(188-advance)//2
                            require(advance<=188-(2 if shadow else 0) and x+left>=0 and x+right+(1 if shadow else 0)<=188,'Date width overflow: '+text)
                            y=(2 if big else 4) if mode=='full' else 4
                            require(y+top>=2,'Date top clipping: '+text)
                            actual_bottom=y+bottom+(1 if shadow else 0)
                            require(actual_bottom<=(40 if mode=='full' else 34),'Date bottom overflow: '+text)
                            gap=minimum_tops[big,mode,shadow]-actual_bottom
                            require(gap>=5,'Date/clock gap too small: '+text)
                            minimum_gap=min(minimum_gap,gap);minimum_date_top=min(minimum_date_top,y+top)
                            date_checks+=1
    context=(ROOT/'src/c/context_text_metrics.h').read_text()
    cps=[int(x,16) for x in re.findall(r'0x([0-9a-f]+)',context.split('s_context_text_codepoints[] = {',1)[1].split('};',1)[0])]
    table=context.split('s_context_text_bottoms[4][205] = {',1)[1].split('};',1)[0]
    contexts=re.findall(r'\{ // (\d+)px\n(.*?)\n  \}',table,re.S)
    require(len(cps)==205 and len(contexts)==4,'Context metric table format changed')
    for raw_size,body in contexts:
        size=int(raw_size);values=[int(x) for x in re.findall(r'\d+',body)]
        ink=read_font(packed,ids['FIRA_SANS_CONDENSED_BOLD_COMPLICATION_'+str(size)],chars=None,ink_bottoms=True)
        require(values==[ink[chr(cp)] for cp in cps],'Context glyph bound drift: '+str(size))
    return {'clockFonts':len(fonts),'clockStrings':len(clocks),'clockChecks':checks,
        'dateStrings':strings,'dateChecks':date_checks,'languages':len(arrays),
        'minimumDateClockGapPx':minimum_gap,'minimumDateTopPx':minimum_date_top,
        'contextGlyphChecks':len(cps)*len(contexts),'nativePayloadMatchesBuild':True}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pbw',type=Path,required=True)
    args=parser.parse_args()
    try:result=verify(args.pbw)
    except (OSError,ValueError,KeyError,struct.error,zipfile.BadZipFile) as error:
        parser.exit(1,'Dynamic font check failed: '+str(error)+'\n')
    print(json.dumps(result,indent=2))
