# Server

This C++ 23 HTTP Server hosts my [own website](http://steinebeisser.de)

Currently, single threaded as my VPS has only 1 vCPU


Tested on an AMD Ryzen 7 7700X 8-Core Processor

```
[3/3] running wrk load against http://localhost:2022/
Running 1m test @ http://localhost:2022/
  4 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.39ms  608.96us   9.00ms   94.10%
    Req/Sec    51.91k     2.06k   61.53k    78.83%
  12400742 requests in 1.00m, 65.29GB read
Requests/sec: 206606.03
Transfer/sec:      1.09GB
```


Tries to set everything as compile flag so runtime is as fast as it can get

Uses SQLite for request stat tracking and IP2location for geo IP, both can be disabled via
the configure script


## GEO IP support

For GEO IP support the IP2Location LITE database is required (free account needed):

1. Register at https://lite.ip2location.com
2. Download at least the DB3 for country and region
3. Place it under `third_party/ip2location-db-bin/IP2LOCATION-LITE-DB3.IPV6.BIN`
