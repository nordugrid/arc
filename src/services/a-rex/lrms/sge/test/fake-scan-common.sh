perflog_common () { :; }

get_owner_uid () {
    id -u
}

do_as_uid () {
    /bin/sh -c "$2"
}

save_commentfile () { :; }
