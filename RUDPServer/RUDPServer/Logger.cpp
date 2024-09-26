#include "PreCompile.h"
#include "Logger.h"

#define LOG_HANDLE  WAIT_OBJECT_0
#define STOP_HANDLE WAIT_OBJECT_0 + 1

Logger::Logger()
{
	for (int i = 0; i < 2; ++i)
	{
		loggerEventHandles[i] = NULL;
	}

	const auto now = std::chrono::system_clock::now();
	std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
	std::tm utcTime;

	auto error = gmtime_s(&utcTime, &currentTime);
	if (error != 0)
	{
		std::cout << "Error in gmtime_s() : " << error << std::endl;
		g_Dump.Crash();
	}

	char buffer[80];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H", &utcTime);

	std::string fileName = std::string("log_") + buffer + ".txt"; // 파일 이름 생성

	logFileStream.open(fileName, std::ios::app);
	if (!logFileStream.is_open()) 
	{
		std::cout << "Logger : Failed to open file " << fileName << std::endl;
		g_Dump.Crash();
	}
}

Logger::~Logger()
{
	logFileStream.close();
}

Logger& Logger::GetInstance()
{
	static Logger inst;
	return inst;
}

void Logger::RunLoggerThread()
{
	loggerEventHandles[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	loggerEventHandles[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (loggerEventHandles[0] == NULL || loggerEventHandles[1] == NULL)
	{
		std::cout << "Logger event handle is invalid" << std::endl;
		g_Dump.Crash();
	}

	loggerThread = std::thread([this]() { this->Worker(); });
}

void Logger::Worker()
{
	std::list<std::shared_ptr<LogBase>> waitingLogList;

	while (true)
	{
		auto result = WaitForMultipleObjects(2, loggerEventHandles, FALSE, INFINITE);
		if (result == LOG_HANDLE)
		{
			WriteLogImpl(waitingLogList);
		}
		else if (result == STOP_HANDLE)
		{
			// 10초간 일단 대기해봄
			Sleep(10000);

			WriteLogImpl(waitingLogList);
			break;
		}
		else
		{
			g_Dump.Crash();
		}

		waitingLogList.clear();
	}
}

void Logger::StopLoggerThread()
{
	SetEvent(loggerEventHandles[1]);

	loggerThread.join();
}

void Logger::WriteLogImpl(std::list<std::shared_ptr<LogBase>>& waitingLogList)
{
	{
		std::lock_guard<std::mutex> guardLock(logQueueLock);

		size_t restSize = logWaitingQueue.size();
		while (restSize > 0)
		{
			waitingLogList.push_back(std::move(logWaitingQueue.front()));
			logWaitingQueue.pop();
		}
	}

	size_t logSize = waitingLogList.size();
	for (auto& logObject : waitingLogList)
	{
		WriteLogToFile(logObject);
	}
}

void Logger::WriteLog(std::shared_ptr<LogBase> logObject)
{
	logObject->SetLogTime();

	std::lock_guard<std::mutex> guardLock(logQueueLock);
	logWaitingQueue.push(logObject);
	
	SetEvent(loggerEventHandles[0]);
}

void Logger::WriteLogToFile(std::shared_ptr<LogBase> logObject)
{
	auto logJson = logObject->ObjectToJsonImpl();
	logFileStream << logJson << '\n';
}