
template <typename T, typename... Args>
struct IsConstructible : BoolConstant<__is_constructible(T, Args...)>
{
};

template <typename T, typename... Args>
constexpr bool IsConstructible_V = __is_constructible(T, Args...);


template <typename T>
struct IsCopyConstructible : BoolConstant<__is_constructible(T, AddLValueReference<const T>::Type)>
{
};

template <typename T>
constexpr bool IsCopyConstructible_V = __is_constructible(T, AddLValueReference<const T>::Type);


template <typename T>
struct IsDefaultConstructible : BoolConstant<__is_constructible(T)>
{
};

template <typename T>
constexpr bool IsDefaultConstructible_V = __is_constructible(T);
