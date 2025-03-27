#include "stdafx.h"
#include "cppThreadPool.h"

CPP_ThreadPool::CPP_ThreadPool()
	:_threadNum(1), _bTerminate(false)//���캯������ʼ�߳���Ϊ1
{

}

// ���������������̳߳�
CPP_ThreadPool::~CPP_ThreadPool()
{
	stop();
}

bool CPP_ThreadPool::init(size_t num)
{
	unique_lock<mutex> lock(_mutex);
	if (!_threads.empty())
		return false;
	_threadNum = num;
	return true;
}

void CPP_ThreadPool::stop()
{
	// ע��Ҫ�����{}����Ȼ��������
	{
		unique_lock<mutex> lock(_mutex);
		_bTerminate = true;
		_condition.notify_all();
	}
	size_t thdCount = _threads.size();
	for (size_t i = 0; i < thdCount; i++)
	{
		if (_threads[i]->joinable())
		{
			_threads[i]->join();
		}
		delete _threads[i];
		_threads[i] = NULL;
	}
	unique_lock<mutex> lock(_mutex);
	_threads.clear();
}

bool CPP_ThreadPool::start()
{
	unique_lock<mutex> lock(_mutex);
	if (!_threads.empty())
		return false;

	for (size_t i = 0; i < _threadNum; i++)
	{
		_threads.push_back(new thread(&CPP_ThreadPool::run, this));
	}
	return true;
}

bool CPP_ThreadPool::reset_num(size_t num)
{
	stop();
	_threadNum = num;
	start();

	return true;
}

bool CPP_ThreadPool::canceltask()
{
	_bCancelTask = true;
	return true;
}

bool CPP_ThreadPool::expansion(size_t num)
{
	//stop();
	//_threadNum = num;
	//start();

	_threadNum += num;
	for (size_t i = 0; i < num; i++)
	{
		_threads.push_back(new thread(&CPP_ThreadPool::run, this));
	}
	return true;
}

bool CPP_ThreadPool::shrinkage(size_t num)
{
	//stop();
	//_threadNum -= num;
	//start();

	
// 	{
// 		unique_lock<mutex> lock(_mutex);
// 		_bTerminate = true;
// 		_condition.notify_all();
// 	}
// 	_threadNum -= num;
// 	for (size_t i = 0; i < num; i++)
// 	{
// 		if (_threads[i]->joinable())
// 		{
// 			_threads[i]->join();
// 		}
// 		delete _threads[i];
// 		_threads[i] = NULL;
// 	}
// 	unique_lock<mutex> lock(_mutex);

	return true;
}

bool CPP_ThreadPool::get(TaskFuncPtr& task)
{
	unique_lock<mutex> lock(_mutex);
	if (_tasks.empty())
	{
		_condition.wait(lock, [this] {
			return _bTerminate || !_tasks.empty();
			});
	}

	if (_bTerminate)
		return false;

	if (!_tasks.empty())
	{
		task = move(_tasks.front());
		_tasks.pop();
		return true;
	}
	return false;
}

// ִ��������߳�
void CPP_ThreadPool::run()
{
	while (!isTerminate())
	{
		TaskFuncPtr task;
		// ��ȡ����
		bool ok = get(task);
		if (ok)
		{
			++_atomic;
			try
			{
				if (task->_expireTime != 0 && task->_expireTime < TNOWMS)
				{
					// ����ʱ����
				}
				else {
					task->_func();//ִ������
				}

			}
			catch (...)
			{
			}
			--_atomic;
			// ����ִ�����,����ֻ��Ϊ��֪ͨwaitForAllDone
			unique_lock<mutex> lock(_mutex);
			if (_atomic == 0 && _tasks.empty())
				_condition.notify_all();
		}
	}
}

bool CPP_ThreadPool::waitForAllDone(int millsecond)
{
	unique_lock<mutex> lock(_mutex);
	if (_tasks.empty())
		return true;
	if (millsecond < 0)
	{
		_condition.wait(lock, [this] { return _tasks.empty(); });
		return true;
	}
	else
	{
		return _condition.wait_for(lock, chrono::milliseconds(millsecond), [this] { return _tasks.empty(); });
	}
}

int gettimeofday(struct timeval& tv)
{
#ifdef WIN32
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tv.tv_sec = clock;
	tv.tv_usec = wtm.wMilliseconds * 1000;
	return 0;
#else
	return ::gettimeofday(&tv, 0);
#endif

}

void getNow(timeval* tv)
{
#if TARGET_PLATFORM_IOS || TARGET_PLATFORM_LINUX 
	int idx = _buf_idx;
	*tv = _t[idx];
	if (fabs(_cpu_cycle - 0) < 0.0001 && _use_tsc)
	{
		addTimeOffset(*tv, idx);
	}
	else
	{
		TC_Common::gettimeofday(*tv);
	}
#else
	gettimeofday(*tv);
#endif 

}

int64_t getNowMs()
{
	struct timeval tv;
	getNow(&tv);
	return tv.tv_sec * (int64_t)1000 + tv.tv_usec / 1000;
}
