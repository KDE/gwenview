This document describe environment variables you can set to debug Gwenview

# `GV_MAX_UNREFERENCED_IMAGES`

How many unreferenced images (images which are not currently displayed and have
not been modified) should be kept in memory.

Defaults to 3

# `GV_THUMBNAIL_DIR`

Defines the dir where thumbnails should be generated.

Defaults to $HOME/.thumbnails/

# `GV_REMOTE_TESTS_BASE_URL`

Used by documenttest. Define a base url where documenttest will put images to
test loading from remote urls.

If not set, remote url tests are skipped.

Should be set to something like `sftp://localhost/tmp`

# `GV_FATAL_FAILS`

If set, when a `GV_RETURN*_IF_FAIL` method fails, then `kFatal()` will be
called, which makes it possible to stop at the place of the failure with the
debugger and also makes it possible for users to report backtraces if they
experiment those failures.
