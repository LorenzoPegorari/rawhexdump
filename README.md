# rawhexdump

**rawhexdump** is a small and limitated copy of *hexdump*, wrote to improve my knowledge of the *C language*, *libc*, *signals*, and how *Unix-like terminals* work.

rawhexdump does not depend on any library.

It uses fairly standard [VT100 escape sequences](https://vt100.net/docs/vt100-ug).

## Resources

The resources that I used to create rawhexdump are the following:

- [snaptoken - Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo)
- [GitHub - antirez/kilo](https://github.com/antirez/kilo)
- [rkoucha - Playing with SIGWINCH](https://www.rkoucha.fr/tech_corner/sigwinch.html)
- [italiancoders - Come scrivere un signal handler in C](https://italiancoders.it/signal-come-si-scrive-un-signal-handler-in-c/)
