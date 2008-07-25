#include <string>
#include <iostream>
#include <unistd.h>
#include <glibmm.h>
#include <db_cxx.h>
#include <arc/Thread.h>

class TestDB
{
    private:
        int counter_;
        DbEnv *env_;
        Db *db_;
    public:
        TestDB(void);
        ~TestDB(void);
        int put(void);
        void list(void);
};

TestDB::TestDB(void)
{
    counter_ = 0;
    env_ = new DbEnv(0); // Exception will occure
    env_->open("db", DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER | DB_THREAD, 0644);
    db_ = new Db(env_, 0);
    db_->open(NULL, "test", NULL, DB_BTREE, DB_CREATE | DB_AUTO_COMMIT | DB_THREAD, 0644);
}

TestDB::~TestDB(void)
{
    std::cout << "Close DB" << std::endl;
    if (db_ != NULL) {
        db_->close(0);
        delete db_;
        db_ = NULL;
    }
    if (env_ != NULL) {
        env_->close(0);
        delete env_;
        env_ = NULL;
    }
}

int TestDB::put(void)
{
    char *foo = "foo";
    Dbt key(&counter_, sizeof(counter_));
    Dbt data(foo, sizeof(foo));
    DbTxn *txn = NULL;
    try {
        unsigned long int id = (unsigned long int)(void*)Glib::Thread::self();
        int ret = env_->txn_begin(NULL, &txn, 0);
        printf("(%d) put(%d): %p\n", id, ret, txn);
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return counter_;
    }
    try {
        db_->put(txn, &key, &data, 0);
        txn->commit(0);
    } catch (DbException &e) {
        std::cerr << "Error in transaction: " << e.what() << std::endl;
        txn->abort();
    }
    txn = NULL;
    return (counter_++);
}

void TestDB::list(void)
{
    Dbc *cursor = NULL;
    DbTxn *txn = NULL;
    env_->txn_begin(NULL, &txn, 0);
    printf("list: %p\n", txn);
    try {
        db_->cursor(txn, &cursor, 0);
        Dbt key, value;
        key.set_flags(0);
        value.set_flags(0);
        int ret;
        std::cout << "Start loop" << std::endl;
        for (;;) {
            ret = cursor->get(&key, &value, DB_NEXT);
            if (ret == DB_NOTFOUND) {
                break;
            }
            int k = *(int *)key.get_data();
            char *v = (char *)value.get_data();
            std::cout << k << " -> " << v << std::endl;  
            // free(key.get_data());
            // free(value.get_data());
            // sleep(1);
        }
        std::cout << "Endloop" << std::endl;
        cursor->close();
        txn->commit(0);
    } catch (DbException &e) {
        std::cerr << "Error in transaction: " << e.what() << std::endl;
        if (cursor != NULL) {
            cursor->close();
        }
        txn->abort();
    }
}

class Writer
{
    private:
        TestDB *db_;
    public:
        Writer(TestDB &db);
        void do_write(void);
};

void Writer::do_write(void)
{
    unsigned long int id = (unsigned long int)(void*)Glib::Thread::self();
    std::cout << "(" << id << ") do_write: " << db_->put() << std::endl;
}

static void writer(void *data)
{
    Writer *w = (Writer *)data;
    Glib::Rand r;
    for (;;) {
        sleep(r.get_int_range(0,3));
        w->do_write();
    }
}

Writer::Writer(TestDB &db)
{
    db_ = &db;
    Arc::CreateThreadFunction(&writer, this);
}

class Reader
{
    private:
        TestDB *db_;
    public:
        Reader(TestDB &db);
        void do_read(void);
};

void Reader::do_read(void)
{
    std::cout << "do_read" << std::endl;
    db_->list();
    std::cout << "do_read done" << std::endl;
}

static void reader(void *data)
{
    Reader *r = (Reader *)data;
    Glib::Rand rnd;
    for (;;) {
        sleep(rnd.get_int_range(0,3));
        r->do_read();
    }
}

Reader::Reader(TestDB &db)
{
    db_ = &db;
    Arc::CreateThreadFunction(&reader, this);
}

int main(void)
{
    TestDB db;
    Writer w1(db);
    Writer w2(db);
    Reader r1(db);
    Reader r2(db);
    sleep(3000);
    return 0;
}
