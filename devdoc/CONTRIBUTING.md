# Intro

Great to hear you want to contribute to Gwenview! Patches are always welcome.

# Mailing list

If you want to discuss development of Gwenview, you can subscribe to
gwenview-devel mailing list:
<https://mail.kde.org/mailman/listinfo/gwenview-devel>.

# Bug tracker

Gwenview bugs are tracked on KDE Bugzilla (<http://bugs.kde.org>). They are
assigned to a fake user by default: `gwenview-bugs-null@kde.org`. To get
notified when new bugs are filed, add this user to the list of users you follow.
You can do so from Bugzilla by editing your user preferences, then go in the
"Email Preferences" tab (<https://bugs.kde.org/userprefs.cgi?tab=email>)

# Code review

Patches should be sent for review on <http://git.reviewboard.kde.org>. You will
get faster answers by posting them there rather than attaching them to a
Bugzilla bug report.

# Commits for stable branch

Commits to stable branch should be made to the stable branch first, then merged
back to master.

Here is an example work-flow.

First fix something in KDE/4.x:

    git checkout KDE/4.x
    # Fix something, get it reviewed
    git commit
    git push

Now merge the commit in master. Note the use of `--no-ff` in `git merge`. This
is required to make it easy to rollback the merge if need be.

    git checkout master
    git merge --no-ff origin/KDE/4.x
    # Check merge is correct
    git push

