#pragma once
#include<Siv3D.hpp>
#include<any>
#include<typeinfo>
#include<typeindex>

/// <summary>
/// type_infoをコピー可能にするラッパクラス
/// </summary>
class Type
{
private:

	const type_info* m_typeInfo;

public:

	Type(const type_info& type)
		:m_typeInfo(&type)
	{

	}

	String name() const
	{
		return Unicode::FromUTF8(m_typeInfo->name());
	}

	const type_info& TypeInfo() const
	{
		return *m_typeInfo;
	}

	bool operator==(const Type& other) const
	{
		return *m_typeInfo == *other.m_typeInfo;
	}

	bool operator!=(const Type& other) const
	{
		return *m_typeInfo != *other.m_typeInfo;
	}

	template<class T>
	static Type getType()
	{
		return Type(typeid(T));
	}

	static Type getType(const std::any& val)
	{
		return Type(val.type());
	}
};