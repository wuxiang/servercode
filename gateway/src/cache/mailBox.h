#ifndef MAILBOX_H_
#define MAILBOX_H_

struct MailBox {
    uint64_t        uniqueID;
    int             fd;

    MailBox(): uniqueID(-1), fd(-1) {
    }

    MailBox(const MailBox& val): uniqueID(val.uniqueID), fd(val.fd) {
    }

    const MailBox& operator=(const MailBox& val) {
        if (this != &val) {
            uniqueID = val.uniqueID;
            fd = val.fd;
        }

        return *this;
    }
};
#endif //MAILBOX_H_

