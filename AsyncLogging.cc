
#include "AsyncLogging.h"
#include "Timestamp.h"

#include <stdio.h>
#include <assert.h>
#include <chrono>
#include <fstream>


using namespace mymuduo;

AsyncLogging::AsyncLogging(const std::string &basename,
						   off_t rollSize,
						   int flushInterval)
	: flushInterval_(flushInterval),
	  running_(false),
	  basename_(basename),
	  rollSize_(rollSize),
	  thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
	  latch_(1),
	  mutex_(),
	  cond_(),
	  currentBuffer_(new Buffer),
	  nextBuffer_(new Buffer),
	  buffers_()
{
	currentBuffer_->bzero();
	nextBuffer_->bzero();
	buffers_.reserve(16);
}

void AsyncLogging::append(const char *logline, int len)
{

	std::unique_lock<std::mutex> lock(mutex_);
	if (currentBuffer_->avail() > len)
	{
		currentBuffer_->append(logline, len);
	}
	else
	{
		buffers_.push_back(std::move(currentBuffer_));

		if (nextBuffer_)
		{
			currentBuffer_ = std::move(nextBuffer_);
		}
		else
		{
			currentBuffer_.reset(new Buffer); // Rarely happens
		}
		currentBuffer_->append(logline, len);
		cond_.notify_one();
	}
}

void AsyncLogging::threadFunc()
{
	latch_.countDown();
	std::string file_name = basename_ + Timestamp::now().toFormattedString(false);
	std::ofstream output;
	output.open(file_name, std::ios::app);
	// LogFile output(basename_, rollSize_, false);
	
	BufferPtr newBuffer1(new Buffer);
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	while (running_)
	{
		// std::assert(newBuffer1 && newBuffer1->length() == 0);
		// assert(newBuffer2 && newBuffer2->length() == 0);
		// assert(buffersToWrite.empty());

		{
			std::unique_lock<std::mutex> lock(mutex_);
			if (buffers_.empty()) // unusual usage!
			{
				cond_.wait_for(lock, std::chrono::seconds(1)*flushInterval_, 
						[&] { 	return !buffers_.empty();	 });
			}
			buffers_.push_back(std::move(currentBuffer_));
			currentBuffer_ = std::move(newBuffer1);
			buffersToWrite.swap(buffers_);
			if (!nextBuffer_)
			{
				nextBuffer_ = std::move(newBuffer2);
			}
		}

		// assert(!buffersToWrite.empty());

		if (buffersToWrite.size() > 25)
		{
			char buf[256];
			snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
					 Timestamp::now().toFormattedString().c_str(),
					 buffersToWrite.size() - 2);
			fputs(buf, stderr);
			// output.append(buf, static_cast<int>(strlen(buf)));
			output << buf;
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
		}

		for (const auto &buffer : buffersToWrite)
		{
			// FIXME: use unbuffered stdio FILE ? or use ::writev ?
			// output.append(buffer->data(), buffer->length());
			output << buffer->data();
		}

		if (buffersToWrite.size() > 2)
		{
			// drop non-bzero-ed buffers, avoid trashing
			buffersToWrite.resize(2);
		}

		if (!newBuffer1)
		{
			// assert(!buffersToWrite.empty());
			newBuffer1 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			newBuffer1->reset();
		}

		if (!newBuffer2)
		{
			// assert(!buffersToWrite.empty());
			newBuffer2 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			newBuffer2->reset();
		}

		buffersToWrite.clear();
		output.flush();
	}
	output.flush();
}
