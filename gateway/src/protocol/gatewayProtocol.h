#ifndef  GATEWAYPROTOCOL_H_
#define  GATEWAYPROTOCOL_H_

class GatewayProtocol {
public:
    uint32_t         length;
    uint16_t         svrNO;
    std::string      body;

public:
    GatewayProtocol();
    GatewayProtocol(const GatewayProtocol&  val);
    virtual ~GatewayProtocol();
    const GatewayProtocol& operator=(const GatewayProtocol& val);
};


class AppSvrProtocol {
public:
    uint32_t       length;
    uint64_t       sequence;
    uint32_t       version;
    std::string    body;

public:
    AppSvrProtocol();
    AppSvrProtocol(const AppSvrProtocol& val);
    virtual ~AppSvrProtocol();
    const AppSvrProtocol& operator=(const AppSvrProtocol& val);
};

#endif //GATEWAYPROTOCOL_H_

