#include <iostream>
#include <glog/logging.h> // For glog

class GlogWrapper{ // 封装Glog
public:
    GlogWrapper(char* program) {
        google::InitGoogleLogging(program);
        FLAGS_log_dir="/root/Downloads/cnlab/lab6/logs/"; //设置log文件保存路径及前缀
        FLAGS_alsologtostderr = true; //设置日志消息除了日志文件之外是否去标准输出
        FLAGS_colorlogtostderr = true; //设置记录到标准输出的颜色消息（如果终端支持）
        FLAGS_stop_logging_if_full_disk = true;   //设置是否在磁盘已满时避免日志记录到磁盘
        // FLAGS_stderrthreshold=google::WARNING; //指定仅输出特定级别或以上的日志
        google::InstallFailureSignalHandler();
    }
    ~GlogWrapper() { google::ShutdownGoogleLogging(); }
};

int main(int argc, char* argv[]) {
    // 初始化 glog
    auto glog = GlogWrapper(argv[0]);

    // 记录不同级别的日志
    LOG(INFO) << "This is an info message.";
    LOG(WARNING) << "This is a warning message.";
    LOG(ERROR) << "This is an error message.";
    // 相当于LOG(ERROR) + return -1；
    // LOG(FATAL) << "This is a fatal message. The program will terminate after this message.";

    // 条件日志
    int x = 2;
    LOG_IF(INFO, x % 2 == 0) << "x is even.";
    LOG_IF(INFO, x % 2 != 0) << "x is odd.";

    // 每n次循环记录一条日志
    for (int i = 0; i < 5; ++i) { LOG_EVERY_N(INFO, x) << "Log every " << x << " iter, current iter：" << i; }
    // 触发段错误，演示崩溃处理效果
    int *p = nullptr;
    p[0] = 0;
}