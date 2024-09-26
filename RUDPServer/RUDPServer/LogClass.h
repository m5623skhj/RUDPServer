#pragma once
#include "nlohmann/json.hpp"

class Logger;

#define OBJECT_TO_JSON_LOG(...)\
    virtual nlohmann::json ObjectToJson() override {\
        nlohmann::json jsonObject;\
        { __VA_ARGS__ }\
        return jsonObject;\
    }\

#define SET_LOG_ITEM(logObject){\
    jsonObject[#logObject] = logObject;\
}

class LogBase
{
	friend Logger;

public:
	LogBase() = default;
	virtual ~LogBase() = default;

public:
	virtual nlohmann::json ObjectToJson() = 0;

private:
	nlohmann::json ObjectToJsonImpl();
	void SetLogTime();

public:
	void SetLastErrorCode(DWORD inLastErrorCode)
	{
		lastErrorCode = inLastErrorCode;
	}

private:
	std::string loggingTime;
	DWORD lastErrorCode = 0;
};

class ServerLog : public LogBase
{
public :
	OBJECT_TO_JSON_LOG(
		SET_LOG_ITEM(logString);
	);

public:
	std::string logString;
};