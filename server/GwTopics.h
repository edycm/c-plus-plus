/**
 * Copyright (c) 2020. All rights reserved.
 * Define topics for gateway
 * @author  tesion
 * @date    2020-09-22
 */
#ifndef IOT_GW_TOPIC_H_
#define IOT_GW_TOPIC_H_

#define GW_REGISTER_UP_TOPIC                "dev.iot.register"
#define GW_LOGIN_UP_TOPIC                   "dev.iot.login"
#define GW_LOGOUT_UP_TOPIC                  "dev.iot.logout"
#define GW_EVENT_UP_TOPIC                   "dev.iot.event"
#define GW_STAT_UP_TOPIC                    "dev.iot.stat"
#define GW_SERVICE_UP_TOPIC                 "dev.iot.service"
#define GW_PROP_SET_UP_TOPIC                "dev.iot.property.set"
#define GW_PROP_GET_UP_TOPIC                "dev.iot.property.get"

#define GW_REG_REPLY_DOWN_TOPIC_FMT         "dev.{PK}.{SN}.iot.register_reply"
#define GW_LOGIN_REPLY_DOWN_TOPIC_FMT       "dev.{PK}.{SN}.iot.login_reply"
#define GW_LOGOUT_REPLY_DOWN_TOPIC_FMT      "dev.{PK}.{SN}.iot.logout_reply"
#define GW_SERVICE_DOWN_TOPIC_FMT           "dev.{PK}.{SN}.iot.service"
#define GW_PROP_SET_DOWN_TOPIC_FMT          "dev.{PK}.{SN}.iot.property.set"
#define GW_PROP_GET_DOWN_TOPIC_FMT          "dev.{PK}.{SN}.iot.property.get"

#endif  // IOT_GW_TOPIC_H_
