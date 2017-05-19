//
// Created by nirao on 17.03.17.
//

#ifndef M66TYPES_H
#define M66TYPES_H

//enum QIOPEN{
//
//};
//
//enum CMEERRORS{
//
//};

enum QISTATUS{
    IP_INITIAL = 0,   //0
    IP_START,         //1
    IP_CONFIG,        //2
    IP_IND,           //3
    IP_GPRSACT,       //4
    IP_STATUS,        //5
    TCP_CONNECTING,   //6
    IP_CLOSE,         //7
    CONNECT_OK,       //8
    PDP_DEACT,        //9
};
//const char *QIStatusStr[11] = {"IP INITIAL",
//                              "IP START",
//                              "IP CONFIG",
//                              "IP IND",
//                              "IP GPRSACT",
//                              "IP STATUS",
//                              "TCP CONNECTING",
//                              "UDP CONNECTING",
//                              "IP CLOSE",
//                              "CONNECT OK",
//                              "PDP DEACT" };


/*what if +PDP DEACT*/

/**/
#endif //M66TYPES_H
