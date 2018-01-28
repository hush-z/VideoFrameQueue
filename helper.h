#include <queue>
#include <thread>
#include <mutex>
#include <stdint.h>
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

class MatData
{
	public:
		struct MatDataFlatMembers {
			int64_t timestamp = 0;
			int64_t frameNum = 0;
		};

		MatDataFlatMembers vars;
		Mat ir, depth, bgr;

		MatData() { }
		~MatData() { }

		MatData(const MatData& src) {
			Clone(src);//call clone Mat
		}

		MatData& operator=(const MatData&src)
		{
			if (this == &src) return *this;
			Copy(src);
			return *this;
		}

		// this is just to collect stats
		unsigned char* GetMatPointer(){ return ir.data; }

	private:
		// USED BY =operator
		void Copy(const MatData& src)
		{
			vars = src.vars; //Copy all flat members using struct copy
			ir = src.ir;
			depth = src.depth;
			bgr = src.bgr;
		}

		//USED BY copy constructor
		void Clone(const MatData& src)
		{
			vars = src.vars;//Copy all flat members using struct copy
			src.ir.copyTo(ir);
			src.depth.copyTo(depth);
			src.bgr.copyTo(bgr);
		}
};

template <class T>
class ThreadSafeFIFOBuffer
{
	public:
		// _maxSize=0 :size is unlimited (up to available memory)
		ThreadSafeFIFOBuffer(size_t _maxSize = 20) :
			maxBufSize(0), itemCount(0), maxSize(_maxSize)
	{}
		~ThreadSafeFIFOBuffer()
		{
			while (!m_buffer.empty())
				m_buffer.pop();
		}

		bool Push(const T &item)
		{
			//mutex is automatically released when lock goes out of scope
			lock_guard<mutex> lock(m_queueMtx);

			size_t size = m_buffer.size();
			if (maxSize > 0 && size > maxSize)
				return false;

			m_buffer.push(item);//calls T::copy constructor
#ifdef _DEBUG
			//collect some stats
			itemCount++;
			maxBufSize = max(size, maxBufSize);
			MatData *dataPtr = &m_buffer.back();
			unsigned char *matPtr = m_buffer.back().GetMatPointer();
			dataMemoryCounter[dataPtr]++;
			matMemoryCounter[matPtr]++;
#endif
			return true;
		}

		bool Pop(T &item)
		{
			lock_guard<mutex> lock(m_queueMtx);
			if (m_buffer.empty())
				return false;
			item = m_buffer.front(); //calls T::=operator 
			m_buffer.pop();
			return true;
		}

		// return length after pop, if previous if empty, return -1
		int PopAndCount(T &item){
			lock_guard<mutex> lock(m_queueMtx);
			if(m_buffer.empty())
				return -1;
			item = m_buffer.front();
			m_buffer.pop();
			return m_buffer.size();
		}

		size_t Size() {
			lock_guard<mutex> lock(m_queueMtx);
			return m_buffer.size();
		}
#ifdef _DEBUG
		size_t GetItemCount(){
			lock_guard<mutex> lock(m_queueMtx);
			return itemCount;
		}
		size_t GetBufSizeMax(){
			lock_guard<mutex> lock(m_queueMtx);
			return maxBufSize;
		}
		size_t GetDataMemoryCount(){
			lock_guard<mutex> lock(m_queueMtx);
			return dataMemoryCounter.size();
		}
		size_t GetMatMemoryCount(){
			lock_guard<mutex> lock(m_queueMtx);
			return matMemoryCounter.size();
		}
#endif
	private:
		queue<T> m_buffer;
		mutex m_queueMtx;
		size_t maxBufSize, maxSize;
		size_t itemCount;
		map<void*, int> dataMemoryCounter;
		map<void*, int> matMemoryCounter;
};
