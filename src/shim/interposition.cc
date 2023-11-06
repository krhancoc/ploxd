#define TYPEDEF_INST(return_val, name, ...) \
	typedef return_val (* name)(__VA_ARGS__); \
	name __real_##name = nullptr

extern "C" {

} // EXTERN C
