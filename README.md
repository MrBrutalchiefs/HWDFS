# Introduction
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Based on DDBHW, we implement a prototype of distributed file system (called HWDFS), which uses DDBHW to speed up partial writes. The system can be deployed on multiple machines that are mounted with local file systems (e.g. ext3). The interface of HWDFS is patterned after the UNIX file system, faithfulness to standards is sacrificed for ease of implementation, since the goal of HWDFS is to evaluate the efficiency of DDBHW.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;HWDFS splits file content into fixed-size data chunks (typically 128 MB, but it can be set by users), it stores each chunk at a single data node. HWDFS encodes each k consecutive data chunks of a file to generate m parity chunks. The size of a parity chunk is the same as that of a data chunk, each parity chunk is independently stored on a single parity node. 

# Reference
* Erasure Code:
  * [jerasure](https://github.com/tsuraan/Jerasure)
  * [gf-complete](https://github.com/ceph/gf-complete)
  * [ReedSolomon](https://github.com/MrZander/ReedSolomon)
* [TSCNS](https://github.com/MengRao/tscns)
* [Thrift](https://github.com/apache/thrift)
* [TinyXML](https://github.com/icebreaker/TinyXML)
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
  * you can change update scheme, IP address and port in `config.xml`
