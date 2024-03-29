//
//  main.c
//  copyfile_test
//

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <removefile.h>

#include "bsize_test.h"
#include "clone_test.h"
#include "ctype_test.h"
#include "identical_test.h"
#include "readonly_fd_test.h"
#include "revert_writable_test.h"
#include "sparse_test.h"
#include "stat_test.h"
#include "xattr_test.h"
#include "xdev_test.h"
#include "test_utils.h"

#define TEST_DIR			"/tmp/copyfile_test"

#define MIN_BLOCKSIZE_B		512
#define DEFAULT_BLOCKSIZE_B	4096
#define MAX_BLOCKSIZE_B		16384

int main(__unused int argc, __unused const char * argv[]) {
	bool failed = false;
	struct statfs stb;

	(void)removefile(TEST_DIR, NULL, REMOVEFILE_RECURSIVE);
	assert_no_err(mkdir(TEST_DIR, 0777));

	// Make sure the test directory exists, is apfs formatted,
	// and that we have a sane block size.
	assert_no_err(statfs(TEST_DIR, &stb));
	assert_no_err(memcmp(stb.f_fstypename, APFS_FSTYPE, sizeof(APFS_FSTYPE)));
	if (stb.f_bsize < MIN_BLOCKSIZE_B || stb.f_bsize > MAX_BLOCKSIZE_B) {
		stb.f_bsize = DEFAULT_BLOCKSIZE_B;
	}

	// Run our tests.
	sranddev();
	failed |= do_readonly_fd_test(TEST_DIR, stb.f_bsize);
	failed |= do_sparse_test(TEST_DIR, stb.f_bsize);
	failed |= do_sparse_recursive_test(TEST_DIR, stb.f_bsize);
	failed |= do_clone_copy_intent_test(TEST_DIR, stb.f_bsize);
	failed |= do_fcopyfile_offset_test(TEST_DIR, stb.f_bsize);
	failed |= do_preserve_dst_flags_test(TEST_DIR, stb.f_bsize);
	failed |= do_preserve_dst_tracked_test(TEST_DIR, stb.f_bsize);
	failed |= do_src_dst_identical_test(TEST_DIR, stb.f_bsize);
	failed |= do_revert_writable_test(TEST_DIR, stb.f_bsize);
	failed |= do_xattr_test(TEST_DIR, stb.f_bsize);
	failed |= do_xattr_flags_test(TEST_DIR, stb.f_bsize);
	failed |= do_bsize_test(TEST_DIR, stb.f_bsize);
	failed |= do_compressed_type_test(TEST_DIR, stb.f_bsize);
	failed |= do_xdev_test(TEST_DIR, stb.f_bsize);

	(void)removefile(TEST_DIR, NULL, REMOVEFILE_RECURSIVE);

	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
