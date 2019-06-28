#include "Sample.h"
#include "Buffer_impl.h"
#include <iterator>
#include <set>
#include "BSDB.h"
using namespace rsdn;
using namespace rsdn::data;

namespace rsdn
{
	constexpr size_t SAMPLECHUNK_DATABUFFERSIZE = 64 * 1024 * 1024;
	constexpr size_t SAMPLECHUNK_LABBUFFERSIZE = 16 * 1024 * 1024;
	constexpr size_t SAMPLECHUNK_OUTBUFFERSIZE = 16 * 1024 * 1024;
	constexpr size_t SAMPLECHUNK_LABLEN = SAMPLECHUNK_LABBUFFERSIZE / sizeof(int);
	constexpr size_t SAMPLECHUNK_OUTLEN = SAMPLECHUNK_OUTBUFFERSIZE / sizeof(datatype);

	class SampleChunk_impl
	{
	public:
		std::vector<std::unique_ptr<FixedBuffer>> data;
		std::vector<std::unique_ptr<FixedBuffer>> lab;
		std::vector<std::unique_ptr<FixedBuffer>> out;
		std::vector<int> dataend;
		std::vector<int> labend;
		std::vector<int> outend;

		sizetype datacapacity;
		int datarowlen;
		sizetype datacount;

		sizetype labcapacity;
		sizetype labcount;

		sizetype outcapacity;
		sizetype outcount;

		int datalen;
		bool uselabel;

		std::vector<int> uniquelabels;

		SampleChunk_impl(): datacapacity(0), datacount(0), labcapacity(0), labcount(0), outcapacity(0), outcount(0), datalen(0), datarowlen(0), uselabel(true)
		{

		}

		void SetFeatureCount(int count)
		{
			datarowlen = count;
			int olddatalen = datalen;
			datalen = (int)floor((double)SAMPLECHUNK_DATABUFFERSIZE / sizeof(datatype) / count);
			if (olddatalen != datalen)
			{
				int datanum = (int)ceil((double)datacount / datalen);
				if (datanum < data.size())
				{
					data.erase(data.begin() + datanum, data.end());
					dataend.erase(dataend.begin() + datanum, dataend.end());
				}
				else
				{
					int addcount = data.size() - datanum;
					int dataend0 = dataend.empty() ? 0 : dataend.back();
					for (int i = 0; i < addcount; i++)
					{
						dataend.push_back(dataend0 + (i + 1)* datalen);
						data.push_back(std::make_unique<FixedBuffer>(SAMPLECHUNK_DATABUFFERSIZE));
					}
				}
				datacapacity = data.size() * datalen;
			}
		}

		void Destroy()
		{
			uniquelabels.clear();

			data.clear();
			lab.clear();
			out.clear();

			dataend.clear();
			labend.clear();
			outend.clear();

			datacapacity = 0;
			datacount = 0;
			labcapacity = 0;
			labcount = 0;
			outcapacity = 0;
			outcount = 0;
		}

		void Resize(sizetype count, bool reserve, bool uselab)
		{
			uselabel = uselab;
			if (uselabel)
			{
				if (outcapacity)
				{
					out.clear();
					outend.clear();
					outcapacity = 0;
					outcount = 0;
				}
				
				int newlabcount = (int)ceil(((double)(count - labcapacity)) / SAMPLECHUNK_LABLEN);
				if (labcapacity < count)
				{
					int addcount = newlabcount - (int)lab.size();
					if (addcount > 0)
					{
						int labend0 = labend.empty() ? 0 : labend.back();
						for (int i = 0; i < addcount; i++)
						{
							labend.push_back(labend0 + (i + 1)* SAMPLECHUNK_LABLEN);
							lab.push_back(std::make_unique<FixedBuffer>(SAMPLECHUNK_LABBUFFERSIZE));
						}
						labcapacity = lab.size() * SAMPLECHUNK_LABLEN;
					}
					labcount = count;
				}
				else
				{
					if (newlabcount > 0)
					{
						lab.erase(lab.begin() + newlabcount, lab.end());
						labend.erase(labend.begin() + newlabcount, labend.end());
						labcapacity = lab.size() * SAMPLECHUNK_LABLEN;
					}
					else
					{
						if (!reserve)
						{
							int alllabcount = (int)ceil(count / SAMPLECHUNK_LABLEN);
							lab.erase(lab.begin() + alllabcount, lab.end());
							labend.erase(labend.begin() + alllabcount, labend.end());
							labcapacity = lab.size() * SAMPLECHUNK_LABLEN;
						}
					}
					labcount = count;
				}
			}
			else
			{
				if (labcapacity)
				{
					lab.clear();
					labend.clear();
					labcapacity = 0;
					labcount = 0;
				}

				int newoutcount = (int)ceil(((double)(count - outcapacity)) / SAMPLECHUNK_OUTLEN);
				if (outcapacity < count)
				{
					int addcount = newoutcount - (int)out.size();
					if (addcount > 0)
					{
						int outend0 = outend.empty() ? 0 : outend.back();
						for (int i = 0; i < addcount; i++)
						{
							outend.push_back(outend0 + (i + 1)* SAMPLECHUNK_OUTLEN);
							out.push_back(std::make_unique<FixedBuffer>(SAMPLECHUNK_OUTBUFFERSIZE));
						}
						outcapacity = out.size() * SAMPLECHUNK_OUTLEN;
					}
					outcount = count;
				}
				else
				{
					if (newoutcount > 0)
					{
						out.erase(out.begin() + newoutcount, out.end());
						outend.erase(outend.begin() + newoutcount, outend.end());
						outcapacity = out.size() * SAMPLECHUNK_OUTLEN;
					}
					else
					{
						if (!reserve)
						{
							int alloutcount = (int)ceil(count / SAMPLECHUNK_OUTLEN);
							out.erase(out.begin() + alloutcount, out.end());
							outend.erase(outend.begin() + alloutcount, outend.end());
							outcapacity = out.size() * SAMPLECHUNK_OUTLEN;
						}
					}
					outcount = count;
				}
			}

			int newdatacount = (int)ceil(((double)(count - datacapacity)) / datalen);
			if (datacapacity < count)
			{
				int addcount = newdatacount - (int)data.size();
				if (addcount > 0)
				{
					int dataend0 = dataend.empty() ? 0 : dataend.back();
					for (int i = 0; i < addcount; i++)
					{
						dataend.push_back(dataend0 + (i + 1)* datalen);
						data.push_back(std::make_unique<FixedBuffer>(SAMPLECHUNK_DATABUFFERSIZE));
					}
					datacapacity = data.size() * datalen;
					
				}
				datacount = count;
			}
			else
			{
				if (newdatacount > 0)
				{
					data.erase(data.begin() + newdatacount, data.end());
					dataend.erase(dataend.begin() + newdatacount, dataend.end());
					datacapacity = data.size() * datalen;
				}
				else
				{
					if (!reserve)
					{
						int alldatacount = (int)ceil(count / datalen);
						data.erase(data.begin() + alldatacount, data.end());
						dataend.erase(dataend.begin() + alldatacount, dataend.end());
						datacapacity = data.size() * datalen;
					}
				}
				datacount = count;
			}
		}

		datatype* GetFeatures(size_t idx)
		{
			int dataidx = -1;
			if (idx < datacount)
			{
				for (size_t i = 0; i < dataend.size(); i++)
				{
					if (idx < dataend[i]) 
					{
						dataidx = i; 
						break;
					}
				}
			}
			if (dataidx == 0) return ((datatype*)data[dataidx]->mutablecpu()) + idx * datarowlen;
			return dataidx != -1 ? ((datatype*)data[dataidx]->mutablecpu()) + (idx - dataend[dataidx - 1]) * datarowlen : nullptr;
		}

		datatype GetFeature(size_t idx, size_t featidx)
		{
			int dataidx = -1;
			if (idx < datacount)
			{
				for (size_t i = 0; i < dataend.size(); i++)
				{
					if (idx < dataend[i])
					{
						dataidx = i;
						break;
					}
				}
			}
			if (dataidx == 0) return (((datatype*)data[dataidx]->mutablecpu()) + idx * datarowlen)[featidx];
			return dataidx != -1 ? (((datatype*)data[dataidx]->mutablecpu()) + (idx - dataend[dataidx - 1]) * datarowlen)[featidx] : 0.0;
		}

		int GetLabel(size_t idx)
		{
			int labidx = -1;
			if (idx < labcount)
			{
				for (size_t i = 0; i < labend.size(); i++)
				{
					if (idx < labend[i])
					{
						labidx = i;
						break;
					}
				}
			}
			if (labidx == 0) return ((int*)lab[labidx]->mutablecpu())[idx];
			return labidx != -1 ? ((int*)lab[labidx]->mutablecpu())[idx - labend[labidx - 1]] : -1;
		}

		void SetLabel(size_t idx, int label)
		{
			int labidx = -1;
			if (idx < labcount)
			{
				for (size_t i = 0; i < labend.size(); i++)
				{
					if (idx < labend[i])
					{
						labidx = i;
						break;
					}
				}
			}
			if (labidx == 0)
			{
				((int*)lab[labidx]->mutablecpu())[idx] = label;
			}
			else if (labidx != -1)
			{
				((int*)lab[labidx]->mutablecpu())[idx - labend[labidx - 1]] = label;
			}
		}

		datatype GetOut(size_t idx)
		{
			int outidx = -1;
			if (idx < outcount)
			{
				for (size_t i = 0; i < outend.size(); i++)
				{
					if (idx < outend[i])
					{
						outidx = i;
						break;
					}
				}
			}
			if (outidx == 0) return ((datatype*)out[outidx]->mutablecpu())[idx];
			return outidx != -1 ? ((datatype*)out[outidx]->mutablecpu())[idx - outend[outidx - 1]] : -1;
		}

		void SetOut(size_t idx, datatype val)
		{
			int outidx = -1;
			if (idx < outcount)
			{
				for (size_t i = 0; i < outend.size(); i++)
				{
					if (idx < outend[i])
					{
						outidx = i;
						break;
					}
				}
			}
			if (outidx == 0)
			{
				((datatype*)out[outidx]->mutablecpu())[idx] = val;
			}
			else if (outidx != -1)
			{
				((datatype*)out[outidx]->mutablecpu())[idx - outend[outidx - 1]] = val;
			}
		}

		datatype* GetAddFeatures(size_t idx)
		{
			int dataidx = -1;
			if (idx < datacapacity)
			{
				for (size_t i = 0; i < dataend.size(); i++)
				{
					if (idx < dataend[i])
					{
						dataidx = i;
						break;
					}
				}
			}
			datatype* ret = nullptr;
			if (dataidx == 0) return ((datatype*)data[dataidx]->mutablecpu()) + idx * datarowlen;
			return dataidx != -1 ? ((datatype*)data[dataidx]->mutablecpu()) + (idx - dataend[dataidx - 1]) * datarowlen : nullptr;
		}

		void SetAddLabel(size_t idx, int label)
		{
			int labidx = -1;
			if (idx < labcapacity)
			{
				for (size_t i = 0; i < labend.size(); i++)
				{
					if (idx < labend[i])
					{
						labidx = i;
						break;
					}
				}
			}
			if (labidx == 0)
			{
				((int*)lab[labidx]->mutablecpu())[idx] = label;
			}
			else if (labidx != -1)
			{
				((int*)lab[labidx]->mutablecpu())[idx - labend[labidx]] = label;
			}
		}

		void SetAddOut(size_t idx, int val)
		{
			int outidx = -1;
			if (idx < outcapacity)
			{
				for (size_t i = 0; i < outend.size(); i++)
				{
					if (idx < outend[i])
					{
						outidx = i;
						break;
					}
				}
			}
			if (outidx == 0)
			{
				((datatype*)out[outidx]->mutablecpu())[idx] = val;
			}
			else if (outidx != -1)
			{
				((datatype*)out[outidx]->mutablecpu())[idx - outend[outidx]] = val;
			}
		}

		void Push_Label(const datatype* features, int label)
		{
			sizetype count = datacount + 1;
			uselabel = true;

			{
				if (outcapacity)
				{
					out.clear();
					outend.clear();
					outcapacity = 0;
					outcount = 0;
				}

				int newlabcount = (int)ceil((double)(count) / SAMPLECHUNK_LABLEN);
				if (labcapacity < count)
				{
					int addcount = newlabcount - (int)lab.size();
					if (addcount > 0)
					{
						int labend0 = labend.empty() ? 0 : labend.back();
						for (int i = 0; i < addcount; i++)
						{
							labend.push_back(labend0 + (i + 1)* SAMPLECHUNK_LABLEN);
							lab.push_back(std::make_unique<FixedBuffer>(SAMPLECHUNK_LABBUFFERSIZE));
						}
						labcapacity = lab.size() * SAMPLECHUNK_LABLEN;
						if (labend.size() == 1)
						{
							((int*)lab.back()->mutablecpu())[labcount] = label;
						}
						else
						{
							((int*)lab.back()->mutablecpu())[labcount - labend[labend.size()-2]] = label;
						}
					}
					labcount = count;
				}
				else
				{
					if (lab.size() > newlabcount)
					{
						lab.erase(lab.begin() + newlabcount, lab.end());
						labend.erase(labend.begin() + newlabcount, labend.end());
						labcapacity = lab.size() * SAMPLECHUNK_LABLEN;
					}					
					SetAddLabel(labcount, label);
					labcount = count;
				}
			}

			int newdatacount = (int)ceil((double)(count) / datalen);
			if (datacapacity < count)
			{
				int addcount = newdatacount - (int)data.size();
				if (addcount > 0)
				{
					int dataend0 = dataend.empty() ? 0 : dataend.back();
					for (int i = 0; i < addcount; i++)
					{
						dataend.push_back(dataend0 + (i + 1)* datalen);
						data.push_back(std::make_unique<FixedBuffer>(SAMPLECHUNK_DATABUFFERSIZE));
					}
					datacapacity = data.size() * datalen;

					if (dataend.size() == 1)
					{
						memcpy(((datatype*)data.back()->mutablecpu()) + (datacount) * datarowlen, features, sizeof(datatype)* datarowlen);
					}
					else
					{
						memcpy(((datatype*)data.back()->mutablecpu()) + (datacount - dataend[dataend.size()-2]) * datarowlen, features, sizeof(datatype)* datarowlen);
					}
				}
				datacount = count;
			}
			else
			{
				if (data.size() > newdatacount)
				{
					data.erase(data.begin() + newdatacount, data.end());
					dataend.erase(dataend.begin() + newdatacount, dataend.end());
					datacapacity = data.size() * datalen;
				}
				memcpy(GetAddFeatures(datacount), features, sizeof(datatype)* datarowlen);
				datacount = count;
			}
		}

		void Push_Output(const datatype* features, datatype val)
		{
			sizetype count = datacount + 1;
			uselabel = false;

			{
				if (labcapacity)
				{
					lab.clear();
					labend.clear();
					labcapacity = 0;
					labcount = 0;
				}

				int newoutcount = (int)ceil(((double)(count)) / SAMPLECHUNK_OUTLEN);
				if (outcapacity < count)
				{
					int addcount = newoutcount - out.size();
					if (addcount > 0)
					{
						int outend0 = outend.empty() ? 0 : outend.back();
						for (int i = 0; i < addcount; i++)
						{
							outend.push_back(outend0 + (i + 1)* SAMPLECHUNK_OUTLEN);
							out.push_back(std::make_unique<FixedBuffer>(SAMPLECHUNK_OUTBUFFERSIZE));
						}
						outcapacity = out.size() * SAMPLECHUNK_OUTLEN;
						if (outend.size() == 1)
						{
							((datatype*)out.back()->mutablecpu())[outcount] = val;
						}
						else
						{
							((datatype*)out.back()->mutablecpu())[outcount - outend[outend.size()-1]] = val;
						}
					}
					outcount = count;
				}
				else
				{
					if (out.size() > newoutcount)
					{
						out.erase(out.begin() + newoutcount, out.end());
						outend.erase(outend.begin() + newoutcount, outend.end());
						outcapacity = out.size() * SAMPLECHUNK_OUTLEN;
					}
					SetAddOut(outcount, val);
					outcount = count;
				}
			}

			int newdatacount = (int)ceil(((double)(count)) / datalen);
			if (datacapacity < count)
			{
				int addcount = newdatacount - (int)data.size();
				if (addcount > 0)
				{
					int dataend0 = dataend.empty() ? 0 : dataend.back();
					for (int i = 0; i < addcount; i++)
					{
						dataend.push_back(dataend0 + (i + 1)* datalen);
						data.push_back(std::make_unique<FixedBuffer>(SAMPLECHUNK_DATABUFFERSIZE));
					}
					datacapacity = data.size() * datalen;
					memcpy(((datatype*)data.back()->mutablecpu()) + (datacount - dataend.back()) * datarowlen, features, sizeof(datatype)* datarowlen);
				}
				datacount = count;
			}
			else
			{
				if (data.size() > newdatacount)
				{
					data.erase(data.begin() + newdatacount, data.end());
					dataend.erase(dataend.begin() + newdatacount, dataend.end());
					datacapacity = data.size() * datalen;
				}
				memcpy(GetAddFeatures(datacount), features, sizeof(datatype)* datarowlen);
				datacount = count;
			}
		}
	};
}

SampleChunk::SampleChunk()
{
	impl = new SampleChunk_impl();
}

SampleChunk::~SampleChunk()
{
	SafeDelete(impl);
}

void SampleChunk::SetFeatureCount(sizetype featcount)
{
	impl->SetFeatureCount(featcount);
}

void SampleChunk::Resize(sizetype count, bool reserve, bool uselabel)
{
	impl->Resize(count, reserve, uselabel);
}

void SampleChunk::PushWithLabel(const datatype* features, int label)
{
	impl->Push_Label(features, label);
}

void SampleChunk::PushWithOutput(const datatype* features, int out)
{
	impl->Push_Output(features, out);
}

sizetype SampleChunk::GetCount() const
{
	return impl->datacount;
}

sizetype SampleChunk::GetFeatureCount() const
{
	return impl->datarowlen;
}

datatype* SampleChunk::GetFeatures(size_t idx) const
{
	return impl->GetFeatures(idx);
}

bool SampleChunk::GetUseLabel() const
{
	return impl->uselabel;
}

int SampleChunk::GetLabel(size_t idx) const
{
	return impl->GetLabel(idx);
}

void SampleChunk::SetLabel(size_t idx, int label)
{
	impl->SetLabel(idx, label);
}

datatype SampleChunk::GetOutput(size_t idx) const
{
	return impl->GetOut(idx);
}

void SampleChunk::SetOutput(size_t idx, datatype val)
{
	impl->SetOut(idx, val);
}

bool SampleChunk::Save(const std::wstring& path)
{
	BSDBWriterPtr writer = std::make_shared<BSDBWriter>();
	if (writer->Open(path, L"rsdn::data::SampleChunk", 1))
	{
		std::vector<int> uniquelabels;

		long long datacapacity = impl->datacapacity;
		int datarowlen = impl->datarowlen;
		long long datacount = impl->datacount;

		long long labcapacity = impl->labcapacity;
		long long labcount = impl->labcount;

		long long outcapacity = impl->outcapacity;
		long long outcount = impl->outcount;

		unsigned long long datalen = impl->datalen;
		bool uselabel = impl->uselabel;

		long long uniquelabelcount = (long long)impl->uniquelabels.size();

		writer->WriteItem(L"datacapacity", &datacapacity, sizeof(long long));
		writer->WriteItem(L"datarowlen", &datarowlen, sizeof(int));
		writer->WriteItem(L"datacount", &datacount, sizeof(long long));
		writer->WriteItem(L"labcapacity", &labcapacity, sizeof(long long));
		writer->WriteItem(L"labcount", &labcount, sizeof(long long));
		writer->WriteItem(L"outcapacity", &outcapacity, sizeof(long long));
		writer->WriteItem(L"outcount", &outcount, sizeof(long long));
		writer->WriteItem(L"datafeatlen", &datalen, sizeof(unsigned long long));
		writer->WriteItem(L"uselabel", &uselabel, sizeof(bool));	
		writer->WriteItem(L"uniquelabelcount", &uniquelabelcount, sizeof(long long));
		if (uniquelabelcount > 0) writer->WriteItem(L"uniquelabels", impl->uniquelabels.data(), sizeof(int)* uniquelabelcount);
		int datachunkcount = (int)impl->data.size();
		writer->WriteItem(L"datachunkcount", &datachunkcount, sizeof(int));
		if (datachunkcount > 0) writer->WriteItem(L"dataend", impl->dataend.data(), sizeof(int)* datachunkcount);
		if (uselabel)
		{
			int labchunkcount = (int)impl->lab.size();
			writer->WriteItem(L"labchunkcount", &labchunkcount, sizeof(int));
			if(labchunkcount > 0) writer->WriteItem(L"labend", impl->labend.data(), sizeof(int)* labchunkcount);
		}
		else
		{
			int outchunkcount = (int)impl->out.size();
			writer->WriteItem(L"outchunkcount", &outchunkcount, sizeof(int));
			if(outchunkcount > 0) writer->WriteItem(L"outend", impl->outend.data(), sizeof(int)* outchunkcount);
		}
		writer->FinishItems();
		int totalchunkcount = datachunkcount + (uselabel ? (int)impl->lab.size() : (int)impl->out.size());
		writer->ReserveChunks(totalchunkcount);

		{
			for (int i = 0; i < datachunkcount; i++)
			{
				writer->WriteChunk(impl->data[i]->getcpu(), SAMPLECHUNK_DATABUFFERSIZE);
			}
		}

		if (uselabel)
		{
			int labchunkcount = (int)impl->lab.size();
			for (int i = 0; i < labchunkcount; i++)
			{
				writer->WriteChunk(impl->lab[i]->getcpu(), SAMPLECHUNK_LABBUFFERSIZE);
			}
		}
		else
		{
			int outchunkcount = (int)impl->out.size();
			for (int i = 0; i < outchunkcount; i++)
			{
				writer->WriteChunk(impl->out[i]->getcpu(), SAMPLECHUNK_OUTBUFFERSIZE);
			}
		}
		writer->FinishChunks();
		writer->Close();
		return true;
	}
	return false;
}

bool SampleChunk::Load(const std::wstring& path)
{
	BSDBReaderPtr reader = std::make_shared<BSDBReader>();
	if (reader->Load(path, [](const std::wstring& header, int) { return header.compare(L"rsdn::data::SampleChunk") == 0; }))
	{
		long long datacapacity = 0;
		int datarowlen = 0;
		long long datacount = 0;

		long long labcapacity = 0;
		long long labcount = 0;

		long long outcapacity = 0;
		long long outcount = 0;

		unsigned long long datafeatlen = 0;
		bool uselabel = true;

		long long uniquelabelcount = 0;

		impl->Destroy();

		reader->GetItemValue(L"datacapacity", &datacapacity);
		reader->GetItemValue(L"datarowlen", &datarowlen);
		reader->GetItemValue(L"datacount", &datacount);

		reader->GetItemValue(L"labcapacity", &labcapacity);
		reader->GetItemValue(L"labcount", &labcount);

		reader->GetItemValue(L"outcapacity", &outcapacity);
		reader->GetItemValue(L"outcount", &outcount);

		reader->GetItemValue(L"datafeatlen", &datafeatlen);

		reader->GetItemValue(L"uselabel", &uselabel);

		reader->GetItemValue(L"uniquelabelcount", &uniquelabelcount);

		if (uniquelabelcount > 0)
		{
			impl->uniquelabels.resize(uniquelabelcount);
			reader->GetItemValues(L"uniquelabels", impl->uniquelabels.data(), uniquelabelcount);
		}

		int datachunkcount = 0;
		reader->GetItemValue(L"datachunkcount", &datachunkcount);
		if (datachunkcount > 0)
		{
			impl->dataend.resize(datachunkcount);
			reader->GetItemValues(L"dataend", impl->dataend.data(), datachunkcount);
		}

		int totalchunkcount = datachunkcount;

		if (uselabel)
		{
			int labchunkcount = 0;
			reader->GetItemValue(L"labchunkcount", &labchunkcount);
			totalchunkcount += labchunkcount;
			impl->labend.resize(labchunkcount);
			reader->GetItemValues(L"labend", impl->labend.data(), labchunkcount);
		}
		else
		{
			int outchunkcount = 0;
			reader->GetItemValue(L"outchunkcount", &outchunkcount);
			totalchunkcount += outchunkcount;
			impl->outend.resize(outchunkcount);
			reader->GetItemValues(L"outend", impl->outend.data(), outchunkcount);
		}

		BinaryBufferPtr chunkptr = std::make_shared<BinaryBuffer>();
		{
			for (int i = 0; i < datachunkcount; i++)
			{	
				if (reader->Read(chunkptr, i))
				{
					auto databufptr = std::make_unique<FixedBuffer>(SAMPLECHUNK_DATABUFFERSIZE);
					memcpy(databufptr->mutablecpu(), chunkptr->GetCpu(), SAMPLECHUNK_DATABUFFERSIZE);
					impl->data.push_back(std::move(databufptr));
				}
			}
		}

		if (uselabel)
		{
			for (int i = datachunkcount; i < totalchunkcount; i++)
			{
				if (reader->Read(chunkptr, i))
				{
					auto labbufptr = std::make_unique<FixedBuffer>(SAMPLECHUNK_LABBUFFERSIZE);
					memcpy(labbufptr->mutablecpu(), chunkptr->GetCpu(), SAMPLECHUNK_LABBUFFERSIZE);
					impl->lab.push_back(std::move(labbufptr));
				}
			}
		}
		else
		{
			for (int i = datachunkcount; i < totalchunkcount; i++)
			{
				if (reader->Read(chunkptr, i))
				{
					auto outbufptr = std::make_unique<FixedBuffer>(SAMPLECHUNK_OUTBUFFERSIZE);
					memcpy(outbufptr->mutablecpu(), chunkptr->GetCpu(), SAMPLECHUNK_OUTBUFFERSIZE);
					impl->out.push_back(std::move(outbufptr));
				}
			}
		}

		reader->Close();

		impl->datacapacity = datacapacity;
		impl->datarowlen = datarowlen;
		impl->datacount = datacount;

		impl->labcapacity = labcapacity;
		impl->labcount = labcount;

		impl->outcapacity = outcapacity;
		impl->outcount = outcount;

		impl->datalen = datafeatlen;
		impl->uselabel = uselabel;

		return true;
	}
	return false;
}

FeatureVirtualMapper::FeatureVirtualMapper(): featidx(0), host(nullptr)
{

}

FeatureVirtualMapper1::FeatureVirtualMapper1() : host(nullptr)
{

}

sizetype FeatureVirtualMapper::GetCount() const
{
	return list.size();
}

datatype FeatureVirtualMapper::operator [](size_t idx) const
{
	return host ? host->impl->GetFeature(list[idx], featidx) : 0.0;
}

sizetype FeatureVirtualMapper1::GetCount() const
{
	return list.size();
}

datatype* FeatureVirtualMapper1::operator [](size_t idx) const
{
	return host ? host->impl->GetFeatures(list[idx]) : nullptr;
}

const std::vector<int>& SampleChunk::GetUniqueLabels() const
{
	return impl->uniquelabels;
}

void SampleChunk::AddUniqueLabel(int label)
{
	impl->uniquelabels.push_back(label);
}

void SampleChunk::ClearUniqueLabel()
{
	impl->uniquelabels.clear();
}

std::unique_ptr<FeatureVirtualMapper> SampleChunk::QueryFeatureAt(sizetype featidx, const std::vector<sizetype>& indexs) const
{
	std::unique_ptr<FeatureVirtualMapper> mapper = std::make_unique<FeatureVirtualMapper>();
	mapper->host = this;
	mapper->featidx = featidx;
	mapper->list = indexs;
	return mapper;
}

std::unique_ptr<FeatureVirtualMapper> SampleChunk::QueryFeatureAt(sizetype featidx, const std::vector<sizetype>& indexs0, const std::vector<sizetype>& indexs1) const
{
	std::unique_ptr<FeatureVirtualMapper> mapper = std::make_unique<FeatureVirtualMapper>();
	mapper->host = this;
	mapper->featidx = featidx;
	mapper->list = indexs0;
	std::copy(indexs1.begin(), indexs1.end(), std::back_inserter(mapper->list));
	return mapper;
}

std::unique_ptr<FeatureVirtualMapper1> SampleChunk::QueryFeaturesByLabel(int label) const
{
	std::unique_ptr<FeatureVirtualMapper1> mapper = std::make_unique<FeatureVirtualMapper1>();
	mapper->host = this;
	for (sizetype i = 0; i < impl->datacount; i++)
	{
		auto lab = GetLabel(i);
		if (lab == label) mapper->list.push_back(i);
	}
	return mapper;
}

