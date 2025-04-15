#ifdef HAVE_GLIBMM_268

#define FILE_TEST_IS_REGULAR    FileTest::IS_REGULAR
#define FILE_TEST_IS_SYMLINK    FileTest::IS_SYMLINK
#define FILE_TEST_IS_DIR        FileTest::IS_DIR
#define FILE_TEST_IS_EXECUTABLE FileTest::IS_EXECUTABLE
#define FILE_TEST_EXISTS        FileTest::EXISTS

#define MODULE_BIND_LAZY  Module::Flags::LAZY
#define MODULE_BIND_LOCAL Module::Flags::LOCAL

#define ModuleFlags Module::Flags

#endif
