std::string get_hostaddr_getnameinfo()
{
    struct ifaddrs* ifaddr = NULL;
    struct ifaddrs* ifa = NULL;
    int family = 0;
    int s = 0;
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs failed\n");
        exit(EXIT_FAILURE);
    }

    char host[NI_MAXHOST] = {0};
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        family = ifa->ifa_addr->sa_family;
        printf("%s address family: %d%s\n", ifa->ifa_name, family,
                                            (family == AF_PACKET) ? " (AF_PACKET)" :
                                            (family == AF_INET) ?   " (AF_INET)" :
                                            (family == AF_INET6) ?  " (AF_INET6)" : "");

        if ((family == AF_INET || family == AF_INET6) && !(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_UP))
        {
            s = getnameinfo(ifa->ifa_addr, (family == AF_INET ? sizeof(struct sockaddr_in): sizeof(struct sockaddr_in6)), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                perror("getifaddrs failed\n");
                exit(EXIT_FAILURE);
            }
            printf("host: %s\n", host);
            freeifaddrs(ifaddr);
            return host;
        }
    }

    return std::string();
}
