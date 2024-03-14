#include<opencv2/opencv.hpp>
#include<thread>
#include<atomic>
#include<mutex>
#include<thread>//首先要引用多线程的头文件
using namespace cv;
using namespace std;//使用std标准命名空间，因为thread类在std下面，如果不写这句话，下面的thread要这样写：std::thread

void read(VideoCapture cap,string win_name)//读取视频的核心函数
{
Mat frame;
    while (cap.read(frame))//捕获每一帧
{
resizeWindow(win_name, 640, 480); 
imshow(win_name, frame);//显示在窗口名为“win_name”的窗口下显示每一帧
waitKey(25);//延时30ms
}   
}
void read_video1()//读取视频1，为了把视频和窗口名传给核心函数
{
VideoCapture cap("./video/video_demo1.mp4");
string win_name = "show1";
read(cap, win_name);
}
void read_video2()//读取视频2，为了把视频和窗口名传给核心函数
{
//VideoCapture cap("./video/video_demo2.mp4");
//string win_name = "show2";
//read(cap, win_name);
std::cout << "join t3\n";
}
int main()
{
thread t1(read_video1);//开启线程t1
 std::this_thread::sleep_for(std::chrono::seconds(1));
thread t2(read_video2);//开启线程t2
  

    std::cout << "join t1\n";
    t1.join(); // 等待 t1 线程结束
    std::cout << "join t2\n";
    t2.join(); // 等待 t2 线程结束

return 0;
}