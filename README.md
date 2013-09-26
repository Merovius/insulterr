insulterr - insult the user with every errormessage
===================================================

I had a discussion earlier about modern \*nix-users using stuff like automatic
zsh typo correction or capitalisation-independent completion. During this
discussion I said something along the lines of „I want my shell to tell me,
when I have a typo. I want it to tell me, what an idiot I have been and that I
suck, so I don't repeat my mistakes“. This is now finally possible.

insulterr is a small LD\_PRELOAD-library, that replaces funktions like
`error(3)`, `strerror(3)` or `perror(3)` to append an insult to every
error-message. Using it globally in your session makes every tools using one of
these functions (which is at least everything in the linux corutils) append an
insult to error-messages like „no such file or directory“.

insulterr looks up a files of localized insults from a directory named in the
environment variable `INSULTERR_DIR` (the locale is not used yet). In the file
there should be one insult per line (with a terminating newline). insulterr
will then choose a random error message from this file on every error.


Building and installing
=======================

```sh
$ make
$ mkdir $(HOME)/.insulterr
$ cp insulterr.so $(HOME)/.insulterr/insulterr.so
$ cp insults $(HOME)/.insulterr
$ echo "INSULTERR_DIR=${HOME}/.insulterr/insults" >> ~/.profile
$ echo "LD_PRELOAD=${HOME}/insulterr.so" >> ~/.profile
```

After restarting your shell, you can do something like

```sh
$ cat nonexistentfile
cat: nonexistentfile: No such file or directory, you idiot!
```


TODO
====

There is still a lot of work needed to make this a usefull tool. The following
is a short list of stuff, I intend to add over the next few days/weeks:

* Localized insults (using LC\_MESSAGES to display insults in the preferred
  language of the user
* Replacing error messages of (at least) zsh (for example, unknown commands do
  not trigger an insult yet)
* Better installation
