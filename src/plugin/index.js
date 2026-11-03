'use strict';

var navigation=require('../common/activity-navigation');
Pebble.registerSourceHandler(function(request,respond){
  if(!request||request.category!=='navigation'||request.item!=='presented_activities'){
    respond.error('INVALID_REQUEST');return;
  }
  respond.data(navigation.source(localStorage,Date.now()));
});
