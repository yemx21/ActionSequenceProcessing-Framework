#pragma once
#include "Core_Config.h"
#include <vector>
#include <memory>

namespace rsdn
{
	class SampleChunk_impl;
	class SampleChunk;

	class CORE_API FeatureVirtualMapper
	{
	private:
		friend SampleChunk;
		const SampleChunk* host;
		sizetype featidx;
		std::vector<sizetype> list;
	public:		
		FeatureVirtualMapper();

		sizetype GetCount() const;

		datatype operator [](size_t idx) const;

		_declspec(property(get = GetCount)) sizetype Count;
	};

	class CORE_API FeatureVirtualMapper1
	{
	private:
		friend SampleChunk;
		const SampleChunk* host;
		std::vector<sizetype> list;
	public:
		FeatureVirtualMapper1();

		sizetype GetCount() const;

		datatype* operator [](size_t idx) const;

		_declspec(property(get = GetCount)) sizetype Count;
	};

	class CORE_API SampleChunk
	{
	private:
		friend FeatureVirtualMapper;
		friend FeatureVirtualMapper1;
		SampleChunk_impl* impl;
	public:
		SampleChunk();
		~SampleChunk();

		void SetFeatureCount(sizetype featcount);
		void Resize(sizetype count, bool reserve = false, bool uselabel = true);

		void PushWithLabel(const datatype* features, int label);
		void PushWithOutput(const datatype* features, int out);

		sizetype GetCount() const;
		sizetype GetFeatureCount() const;

		datatype* GetFeatures(size_t idx) const;

		bool GetUseLabel() const;
		int GetLabel(size_t idx) const;
		void SetLabel(size_t idx, int label);

		const std::vector<int>& GetUniqueLabels() const;
		void AddUniqueLabel(int label);
		void ClearUniqueLabel();

		datatype GetOutput(size_t idx) const;
		void SetOutput(size_t idx, datatype label);

		std::unique_ptr<FeatureVirtualMapper> QueryFeatureAt(sizetype featidx, const std::vector<sizetype>& indexs) const;
		std::unique_ptr<FeatureVirtualMapper> QueryFeatureAt(sizetype featidx, const std::vector<sizetype>& indexs0, const std::vector<sizetype>& indexs1) const;

		std::unique_ptr<FeatureVirtualMapper1> QueryFeaturesByLabel(int label) const;

		bool Save(const std::wstring& path);
		bool Load(const std::wstring& path);

		_declspec(property(get = GetUseLabel)) bool UseLabel;
		_declspec(property(get = GetCount)) sizetype Count;
		_declspec(property(get = GetFeatureCount)) sizetype FeatureCount;

		_declspec(property(get = GetFeatures)) datatype* Features[];
		_declspec(property(get = GetLabel, put = SetLabel)) int Label[];
		_declspec(property(get = GetOutput, put = SetOutput)) datatype Output[];

		_declspec(property(get = GetUniqueLabels)) const std::vector<int>& UniqueLabels;
	};

}
