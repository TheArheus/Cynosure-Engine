#pragma once

struct type_descriptor
{
	const char* Name;
	size_t Size;

	type_descriptor(const char* NewName, size_t NewSize) : Name(NewName), Size(NewSize) {}
	virtual ~type_descriptor() {}
	virtual std::string GetFullName() const { return Name; }
};

template<typename T>
type_descriptor* GetPrimitiveDescriptor();

struct default_resolver
{
	template<typename T> static char func(decltype(&T::Reflection));
	template<typename T> static int func(...);
	template<typename T>
	struct IsReflected
	{
		enum { value = (sizeof(func<T>(nullptr)) == sizeof(char)) };
	};

	template<typename T, typename std::enable_if<IsReflected<T>::value, int>::type = 0>
	static type_descriptor* Get()
	{
		return &T::Reflection;
	}

	template<typename T, typename std::enable_if<!IsReflected<T>::value, int>::type = 0>
	static type_descriptor* Get()
	{
		return GetPrimitiveDescriptor<T>();
	}
};

template<typename T>
struct type_resolver
{
	static type_descriptor* Get()
	{
		return default_resolver::Get<T>();
	}
};

struct type_descriptor_struct : type_descriptor
{
	struct member
	{
		const char* Name;
		size_t Offset;
		type_descriptor* Type;
	};

	std::vector<member> Members;

	type_descriptor_struct(void (*Init)(type_descriptor_struct*)) : type_descriptor(nullptr, 0) { Init(this); }
	type_descriptor_struct(const char* Name, size_t Size, const std::initializer_list<member>& Init) : type_descriptor(nullptr, 0), Members{Init} {}
};

struct type_descriptor_std_vector : type_descriptor
{
	type_descriptor* ItemType;
	size_t (*GetSize)(const void*);
	const void* (*GetItem)(const void*, size_t);

	template<typename item_type>
	type_descriptor_std_vector(item_type*) : type_descriptor("std::vector<>", sizeof(std::vector<item_type>)), ItemType(type_resolver<item_type>::Get())
	{
		GetSize = [](const void* VecPtr) -> size_t
		{
			const auto& Vec = *(const std::vector<item_type>*)VecPtr;
			return Vec.size();
		};
		GetItem = [](const void* VecPtr, size_t Idx) -> const void*
		{
			const auto& Vec = *(const std::vector<item_type>*)VecPtr;
			return &Vec[Idx];
		};
	}

	virtual std::string GetFullName() const override 
	{
		return std::string("std::vector<") + ItemType->GetFullName() + ">";
	}
};

template<typename T>
struct type_resolver<std::vector<T>> 
{
	static type_descriptor* Get()
	{
		static type_descriptor_std_vector TypeDesc((T*)nullptr);
		return &TypeDesc;
	}
};

struct type_descriptor_char : type_descriptor
{
	type_descriptor_char() : type_descriptor("char", sizeof(char)) {}
};

template<>
type_descriptor* GetPrimitiveDescriptor<char>()
{
	static type_descriptor_char TypeDesc;
	return &TypeDesc;
}

struct type_descriptor_short : type_descriptor
{
	type_descriptor_short() : type_descriptor("short", sizeof(short)) {}
};

template<>
type_descriptor* GetPrimitiveDescriptor<short>()
{
	static type_descriptor_short TypeDesc;
	return &TypeDesc;
}

struct type_descriptor_int : type_descriptor
{
	type_descriptor_int() : type_descriptor("int", sizeof(int)) {}
};

template<>
type_descriptor* GetPrimitiveDescriptor<int>()
{
	static type_descriptor_int TypeDesc;
	return &TypeDesc;
}

struct type_descriptor_float : type_descriptor
{
	type_descriptor_float() : type_descriptor("float", sizeof(float)) {}
};

template<>
type_descriptor* GetPrimitiveDescriptor<float>()
{
	static type_descriptor_float TypeDesc;
	return &TypeDesc;
}

struct type_descriptor_double : type_descriptor
{
	type_descriptor_double() : type_descriptor("double", sizeof(double)) {}
};

template<>
type_descriptor* GetPrimitiveDescriptor<double>()
{
	static type_descriptor_double TypeDesc;
	return &TypeDesc;
}

#define REFLECT() \
	friend struct default_resolver; \
	static type_descriptor_struct Reflection; \
	static void InitReflection(type_descriptor_struct*);

#define REFLECT_STRUCT_BEGIN(type) \
	type_descriptor_struct type::Reflection{type::InitReflection}; \
	void type::InitReflection(type_descriptor_struct* TypeDesc) { \
		using T = type; \
		TypeDesc->Name = #type; \
		TypeDesc->Size = sizeof(T); \
		TypeDesc->Members = {

#define REFLECT_STRUCT_MEMBER(Name) \
		{ #Name, offsetof(T, Name), type_resolver<decltype(T::Name)>::Get()},

#define REFLECT_STRUCT_END() \
		}; \
	}

#define introspect()

#define SHADER_PARAMETER_STRUCT_BEGIN(name)
#define SHADER_PARAMETER_STRUCT_END()

#define SHADER_PARAMETER(type, name)
#define SHADER_PARAMETER_WITH_COUNTER(type, name)

#define SHADER_PARAMETER_ARRAY(type, name, count)
#define SHADER_PARAMETER_ARRAY_WITH_COUNTER(type, name, count)

#define SHADER_PARAMETER_VECTOR(type, name)
#define SHADER_PARAMETER_VECTOR_WITH_COUNTER(type, name)
