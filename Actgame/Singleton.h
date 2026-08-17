#pragma once

//シングルトンクラス

template<class T>
class Singleton
{
public:
	static inline T& GetInstance()
	{
		static T instance;
		return instance;
	}

protected:
	Singleton() = default; // 外部でのインスタンス作成は禁止
	virtual ~Singleton() = default;

private:
	Singleton& operator=(const Singleton&) = delete;
	Singleton(const Singleton&) = delete;
};
