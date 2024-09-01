#pragma once

enum class IO_POST_ERROR : unsigned char
{
	SUCCESS = 0
};

enum class RIO_OPERATION_TYPE : INT8
{
	OP_ERROR = 0
	, OP_RECV
	, OP_SEND
};