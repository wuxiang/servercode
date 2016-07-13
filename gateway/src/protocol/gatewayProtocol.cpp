#include "gatewayProtocol.h"

GatewayProtocol::GatewayProtocol(): length(0),version(0) {
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
}

AppSvrProtocol::~AppSvrProtocol() {
}

const AppSvrProtocol& AppSvrProtocol::operator=(const AppSvrProtocol& val) {
    if (this != & val) {
    uint32_t       length;
    uint64_t       sequence;
    uint32_t       version;
    std::string    body;
    }

    return *this;
}

