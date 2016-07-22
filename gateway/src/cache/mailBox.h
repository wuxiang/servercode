#ifndef MAILBOX_H_
#define MAILBOX_H_

struct MailBox {
    uint64_t        uniqueID;
    int             sfd;
    int             dfd;

    MailBox(): uniqueID(-1), sfd(-1), dfd(-1) {
    }

    MailBox(const MailBox& val): uniqueID(val.uniqueID), sfd(val.sfd),dfd(val.dfd) {
    }

    const MailBox& operator=(const MailBox& val) {
        if (this != &val) {
            uniqueID = val.uniqueID;
            sfd = val.sfd;
            dfd = val.dfd;
        }

        return *this;
    }
};
#endif //MAILBOX_H_

