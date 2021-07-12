# Introduction
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;HWDFS is a distributed file system for high availability, high performance, and low cost. It is a linux-based file system that provides high reliability and concurrent access through backup and erasure coding technology. Developers can expand the update scheme with custom erasure codes by implementing the API interface provided by the system. At present, the system has implemented some of the most advanced update schemes. HWDFS is mainly designed for large files larger than 64MB.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;The project is still in the development stage, many bugs have not been fixed, and it is still unable to meet all the scenarios that may appear in the real world. If you find them, please feel free to submit bugs. If you have any questions or feedback, please contact and join us, we appreciate your contribution.

# Reference
* Erasure Code:
  * [jerasure](https://github.com/tsuraan/Jerasure)
  * [gf-complete](https://github.com/ceph/gf-complete)
  * [ReedSolomon](https://github.com/MrZander/ReedSolomon)
* [TSCNS](https://github.com/MengRao/tscns)
* [Apache Thrift](https://github.com/apache/thrift)
* [TinyXML](https://github.com/icebreaker/TinyXML)
* [Robin-Map](https://github.com/Tessil/robin-map)
* [MSR Cambridge Traces](http://iotta.snia.org/traces/388)

# Run Code
1. Install Apache Thrift.
2. Build Project.
    ```shell
    mkdir build; cd build;
    cmake ..
    make
    ```
3. Use `./client -h` to view all commands on the client.
4. Use `./data_server` to run data node, use `./parity_server` to run parity node
5. you can change **Update Scheme**, **IP Address** and **Port** in `config.xml`
