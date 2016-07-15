#include "gatewayProtocol.h"

GatewayProtocol::GatewayProtocol(): length(0),svrNO(0) {
}

GatewayProtocol::GatewayProtocol(const GatewayProtocol&  val) {
    this->length = val.length;
    this->svrNO = val.svrNO;
    this->body = val.body;
}

GatewayProtocol::~GatewayProtocol() {
}

const GatewayProtocol& GatewayProtocol::operator=(const GatewayProtocol& val) {
    if (this != &val) {
        this->length = val.length;
        this->svrNO = val.svrNO;
        this->body = val.body;
    }

    return *this;
}

// application protocol
AppSvrProtocol::AppSvrProtocol() {
}

AppSvrProtocol::AppSvrProtocol(const AppSvrProtocol& val) {
    length = val.length;
    sequence = val.sequence;
    version = val.version;
    body = val.body;
}

AppSvrProtocol::~AppSvrProtocol() {
}

const AppSvrProtocol& AppSvrProtocol::operator=(const AppSvrProtocol& val) {
    if (this != & val) {
        length = val.length;
        sequence = val.sequence;
        version = val.version;
        body = val.body;
    }

    return *this;
}

