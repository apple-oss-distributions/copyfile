//
//  test_utils.h
//  copyfile_test
//	Based on the test routines from the apfs project.
//

#ifndef test_utils_h
#define test_utils_h

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "../copyfile.h"

#include <TargetConditionals.h>

#define BSIZE_B					128
#define MAX_DISK_IMAGE_SIZE_MB	1024

#define DEFAULT_NAME_MOD  		999
#define DEFAULT_OPEN_FLAGS		O_CREAT|O_TRUNC|O_RDWR
#define DEFAULT_OPEN_PERM		0666
#define DEFAULT_MKDIR_PERM		0777

#define DISK_IMAGE_PATH			"/tmp/copyfile_test.sparseimage"
#define APFS_FSTYPE				"apfs"
#define DEFAULT_FSTYPE			APFS_FSTYPE

#define AFSCUTIL_PATH			"/usr/local/bin/afscutil"
#define HDIUTIL_PATH			"/usr/bin/hdiutil"
#define DIFF_PATH				"/usr/bin/diff"

// Test routine helpers.
bool verify_times(const char *timename, const struct timespec *expected, const struct timespec *actual);
bool verify_path_missing_xattr(const char *path, const char *xattr_name);
bool verify_path_xattr_content(const char *path, const char *xattr_name, const char *expected,
							   size_t size);
bool verify_fd_xattr_contents(int orig_fd, int copy_fd);
bool verify_st_flags(const struct stat *sb, uint32_t flags_to_check, uint32_t flags_to_expect);
bool verify_st_ids_and_mode(const struct stat *expected, const struct stat *actual);
bool verify_contents_with_buf(int orig_fd, off_t orig_pos, const char *expected, size_t length);
bool verify_fd_contents(int orig_fd, off_t orig_pos, int copy_fd, off_t copy_pos, size_t length);
bool verify_copy_contents(const char *orig_name, const char *copy_name);
bool verify_copy_sizes(const struct stat *orig_sb, const struct stat *copy_sb, copyfile_state_t cpf_state,
					   bool do_sparse, off_t src_start);
int create_hole_in_fd(int fd, off_t offset, off_t length);
void write_compressible_data(int fd);
void compress_file(const char *path, const char *type);
void create_test_file_name(const char *dir, const char *postfix, int id, char *string_out);

#if TARGET_OS_OSX
// Our disk image test functions.
void disk_image_create(const char *fstype, const char *mount_path, size_t size_in_mb);
void disk_image_destroy(const char *mount_path, bool allow_failure);
#endif

// Assertion functions/macros for tests.
static inline void
__attribute__((format (printf, 3, 4)))
__attribute__((noreturn))
assert_fail_(const char *file, int line, const char *assertion, ...)
{
	va_list args;
	va_start(args, assertion);
	char *msg;
	vasprintf(&msg, assertion, args);
	va_end(args);
	printf("\n%s:%u: error: %s\n", file, line, msg);
	exit(1);
}

#define assert_fail(str, ...)						\
	assert_fail_(__FILE__, __LINE__, str, ## __VA_ARGS__)

#undef assert
#define assert(condition) assert_true(condition)

#define assert_true(condition)										\
	do {															\
		if (!(condition))											\
			assert_fail_(__FILE__, __LINE__,						\
						"assertion failed: %s", #condition);		\
	} while (0)

#define assert_with_errno_(condition, condition_str)					\
	do {																\
		if (!(condition))												\
			assert_fail_(__FILE__, __LINE__, "%s failed: %s",			\
						condition_str, strerror(errno));				\
	} while (0)

#define assert_with_errno(condition)			\
	assert_with_errno_((condition), #condition)

#define assert_no_err(condition) \
	assert_with_errno_(!(condition), #condition)

#define assert_fd(expr) \
	assert_with_errno_(((expr) >= 0), #expr)

#define assert_equal(lhs, rhs, fmt)										\
	do {																\
		typeof (lhs) lhs_ = (lhs);										\
		typeof (lhs) rhs_ = (rhs);										\
		if (lhs_ != rhs_)												\
			assert_fail(#lhs " (" fmt ") != " #rhs " (" fmt ")",		\
						lhs_, rhs_);									\
	} while (0)

#define assert_equal_(lhs, rhs, lhs_str, rhs_str, fmt)					\
	do {																\
		typeof (lhs) lhs_ = (lhs);										\
		typeof (lhs) rhs_ = (rhs);										\
		if (lhs_ != rhs_)												\
			assert_fail(lhs_str " (" fmt ") != " rhs_str " (" fmt ")",	\
						lhs_, rhs_);									\
	} while (0)

#define assert_equal_int(lhs, rhs)	assert_equal_(lhs, rhs, #lhs, #rhs, "%d")
#define assert_equal_ll(lhs, rhs)	assert_equal_(lhs, rhs, #lhs, #rhs, "%lld")
#define assert_equal_str(lhs, rhs)										\
	do {																\
		const char *lhs_ = (lhs), *rhs_ = (rhs);						\
		if (strcmp(lhs_, rhs_))											\
			assert_fail("\"%s\" != \"%s\"", lhs_, rhs_);				\
	} while (0)

#define check_io(fn, len) check_io_((fn), len, __FILE_NAME__, __LINE__, #fn)

static inline ssize_t check_io_(ssize_t res, ssize_t len, const char *file,
								int line, const char *fn_str)
{
	if (res < 0)
		assert_fail_(file, line, "%s failed: %s", fn_str, strerror(errno));
	else if (len != -1 && res != len)
		assert_fail_(file, line, "%s != %ld (%ld)", fn_str, len, res);
	return res;
}

//
// assert that a call (likely a syscall) fails with a particular errno
// if the call is permitted to fail with any errno, pass `expected_errno' = 0
//
// this has a strange name to not conflict with preexisting `assert_fail()'
//
#define assert_call_fail_(call_expr, expr_str, expected_errno, ...) \
	do { \
		if ((call_expr) == -1) { \
			int save_errno = errno; \
			if ((expected_errno != 0) && (save_errno != expected_errno)) { \
				assert_fail("%s returned errno %d != %d; '%s' != '%s'", \
						expr_str, save_errno, expected_errno, \
						strerror(save_errno), strerror(expected_errno)); \
			} \
		} else { \
			assert_fail("%s returned success, but should have failed " \
					"with '%s', errno %d", \
					expr_str, \
					(expected_errno) ? \
						strerror(expected_errno) : \
						"*", \
					expected_errno); \
		} \
	} while (0)

/*
 * Permit assert_call_fail_() to be used with no specified errno.
 * Expressions like
 *   assert_call_fail(fcntl(...));
 *   assert_call_fail(fcntl(...), EINVAL);
 * are both valid. The first form will test that fcntl() will fail with any
 * error.
 */
#define assert_call_fail(call_expr, ...) \
	assert_call_fail_((call_expr), #call_expr, ##__VA_ARGS__, 0)

#define ignore_eintr(x, error_val)								\
	({															\
		typeof(x) eintr_ret_;									\
		do {													\
			eintr_ret_ = (x);									\
		} while (eintr_ret_ == (error_val) && errno == EINTR);	\
		eintr_ret_;												\
	})

#endif /* test_utils_h */
