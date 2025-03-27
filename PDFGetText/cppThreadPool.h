#ifndef _CPP_THREAD_POOL_H_
#define _CPP_THREAD_POOL_H_

#include <iostream>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <future>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

using namespace std;

void getNow(timeval* tv);
int64_t getNowMs();

#define TNOW    getNow()
#define TNOWMS  getNowMs()

class CPP_ThreadPool {
protected:
    struct TaskFunc {
        TaskFunc(uint64_t expireTime) :_expireTime(expireTime) {}
        int64_t _expireTime = 0;//��ʱ�ľ���ʱ��
        function<void()> _func;
    };
    typedef shared_ptr<TaskFunc> TaskFuncPtr;

    /*
     * @brief ��ȡ���� **
     *@return TaskFuncPtr
     */
    bool get(TaskFuncPtr& task);

    /*
    * @brief �̳߳��Ƿ��˳�
    */
    bool isTerminate()
    {
        return _bTerminate;
    }

    /*
    * @brief �߳�����̬
    */
    void run();

public:
    /*
     * @brief ���캯��
     */
    CPP_ThreadPool();

    /*
    * @brief ����, ��ֹͣ�����߳�
    */
    virtual ~CPP_ThreadPool();

    /*
     * * @brief ��ʼ��.
     * * @param num �����̸߳���
     */
    bool init(size_t num);

    /*
    * @brief ֹͣ�����߳�, ��ȴ������߳̽���
    */
    void stop();

    /*
    * @brief ���������߳�
    */
    bool start();

    bool expansion(size_t num);
    bool shrinkage(size_t num);
    bool reset_num(size_t num);

    bool canceltask();
    /*
     * @brief �ȴ���ǰ���������, ���й���ȫ������(����������).
     * @param millsecond �ȴ���ʱ��(ms), -1:��Զ�ȴ�
     * @return true, ���й������������
     * false,��ʱ�˳�
     */
    bool waitForAllDone(int millsecond = -1);

    /*
     * @brief ��ȡ�̸߳���.
     * @return size_t �̸߳���
     */
    size_t getThreadNum()
    {
        unique_lock<mutex> lock(_mutex);
        return _threads.size();
    }

    /*
     *  @brief ��ȡ��ǰ�̳߳ص�������
     * @return size_t �̳߳ص�������
     */
    size_t getJobNum()
    {
        unique_lock<mutex> lock(_mutex);
        return _tasks.size();
    }

    /*
    * @brief ���̳߳���������(F��function, Args�ǲ���) **
    * @param ParentFunctor
    * @param tf
    * @return ���������future����, ����ͨ�������������ȡ����ֵ
    */
    template <class F, class... Args>
    auto exec(F&& f, Args&&... args)->future<decltype(f(args...))>
    {
        return exec(0, f, args...);
    }

    /*
    * unused.
    *
    * @brief ���̳߳���������(F��function, Args�ǲ���)
    * @param ��ʱʱ�� ����λms (Ϊ0ʱ������ʱ����) ��������ʱ�������񽫱�����
    * @param bind function
    * @return ���������future����, ����ͨ�������������ȡ����ֵ
    *
    * template <class F, class... Args>
    * ����c++����������ǿ�������֮һ�����Բ��������˸߶ȷ��������ܱ�ʾ0������������������͵Ĳ���
    * auto exec(F &&f, Args &&... args) -> std::future<decltype(f(args...))>
    * std::future<decltype(f(args...))>������future�������߿���ͨ��future��ȡ����ֵ
    * ����ֵ����
    */
    template<class F, class... Args>
    auto exec(int64_t timeoutMs, F&& f, Args&&... args) -> future<decltype(f(args...))>
    {
        //��ȡ����ʱ��
        int64_t expireTime = (timeoutMs == 0) ? 0 : TNOWMS + timeoutMs;
        // ���巵��ֵ����
        using retType = decltype(f(args...));
        // ��װ����
        auto task = make_shared<packaged_task<retType()>>(bind(forward<F>(f), forward<Args>(args)...));
        // ��װ����ָ�룬���ù���ʱ��
        TaskFuncPtr fPtr = make_shared<TaskFunc>(expireTime);
        fPtr->_func = [task]() {
            (*task)();
        };

        unique_lock<mutex> lock(_mutex);
        // ��������
        _tasks.push(fPtr);
        // �����������̣߳����Կ���ֻ���������Ϊ�յ��� ����ȥnotify
        _condition.notify_one();

        return task->get_future();
    }

protected:
    size_t  _threadNum;//�߳�����
    bool    _bTerminate;//�ж��Ƿ���ֹ�̳߳�
    mutex   _mutex;     //Ψһ��
    vector<thread*> _threads;   //�����߳�����
    queue<TaskFuncPtr> _tasks;  //�������
    condition_variable _condition;//��������
    atomic<int>         _atomic{ 0 };//ԭ�ӱ���
    bool    _bCancelTask;
};


#endif