#pragma once

#include <memory>
#include <typeinfo>

#include "Utility/Check.h"

template <typename T> class TSingleton
{
public:
	// 생성 비용이 크거나 생성 시점을 잡아야 하는 싱글톤만 부른다. 나머지는 첫 Get() 에 맡긴다.
	static void Prepare()
	{
		CHECK_RETURN(Instance == nullptr);

		const bool bResurrected = bReleased;
		Instance = std::make_unique<T>();

		if (bResurrected)
		{
			// 인스턴스를 만든 뒤에 알린다. FLog 자신이 되살아난 경우라면 알릴 곳이 방금 생긴 것이다.
			::Return::ReportResurrection(typeid(T).name());
		}
	}

	// 해제 시점이 중요한 싱글톤만 부른다. 부른 뒤에 Get() 이 오면 경고와 함께 다시 만들어진다.
	static void Release()
	{
		Instance.reset();
		bReleased = true;
	}

	static T& Get()
	{
		if (Instance == nullptr)
		{
			Prepare();
		}

		return *Instance;
	}

private:
	static std::unique_ptr<T> Instance;
	static bool bReleased;

protected:
	TSingleton() = default;
	~TSingleton() = default;

	TSingleton(const TSingleton& Other) = delete;
	TSingleton& operator=(const TSingleton& Other) = delete;
};

template <typename T> std::unique_ptr<T> TSingleton<T>::Instance;
template <typename T> bool TSingleton<T>::bReleased = false;
