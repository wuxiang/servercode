#ifndef APPBASE_H_
#define APPBASE_H_
#include <string>

#include <boost/shared_ptr.hpp>

class AppBase {
public:
    AppBase(const uint16_t service, const std::size_t& id): m_service(service), m_id(id) {
    }

    virtual ~AppBase() {
    }

    uint16_t  getServiceName() {
        return m_service;
    }

    std::size_t  getID() {
        return m_id;
    }

protected:
    uint16_t      m_service;
    std::size_t   m_id;
};

typedef boost::shared_ptr<AppBase>    AppBasePtr;
#endif //APPBASE_H_
