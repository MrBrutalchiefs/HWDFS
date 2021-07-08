namespace cpp comm

/**
 * 响应编码枚举
 */
enum ResponseCode {SUCCESS = 1, NEED_D0 = 2, ERROR = 3}

enum DataType {D0 = 1, DR = 2, P0 = 3, PR = 4}

/**
 * 自定义请求体
 */
struct Request {
  1: required i16 from_id,
  2: required i16 to_id,
  3: required string  scheme,

  4: required list<i8> data,
  5: required i64 length,
  6: required i64 offset,
  7: required DataType type,
  8: required i32 partId
}

/**
 * 自定义响应体
 */
struct Response {
  1: required ResponseCode code,
  2: required list<i8> data
}

/**
 * 存储服务类
 * CREATE接口：用于创建数据块和奇偶校验块
 * WRITE接口：用于更新数据块和奇偶校验块
 * READ接口：用于读取数据块和奇偶校验块
 * DEGRADED_READ接口：用于测试系统的恢复性能
 * MERGE接口：用户服务器中数据块与日志的合并
 */
service StoreService {
  Response create(1:Request request),
  Response write(1:Request request),
  Response read(1:Request request),
  Response degrade_read(1:Request request),
  Response merge(1:Request request),
  Response clear(1:Request request)
}