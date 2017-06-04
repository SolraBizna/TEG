This is a set of utilities and portability hacks I've accumulated over the years when writing games. I make no guarantee of backwards/forwards compatibility or usability. This only exists to make _my_ life easier, not yours. :)

Things that might be useful to someone other than me:

- `IO`: Keep your strings and paths in UTF-8, and use this for IO. Only supports C-style `FILE*`-based IO. Handles creation of configuration directories in standard places, and reading of datafiles from an executable-relative directory. This one is likely to be significantly redone in the future.
- `Net`: Classes for (mostly) non-blocking socket IO. Works on Windows through the magic of the C preprocessor, and Unices through the magic of _people following the standards_. Supports TCP and UDP over IPv4 and IPv6.
- `xgl`: There are other libraries to do this, but none of them are mine. Specify exactly the OpenGL extensions your application cares about (as long as they're extensions I've cared about), and this will handle detecting their presence, dynamically loading the functions, etc. Will try to use/adapt near-equivalent extensions where possible. Like most of my OpenGL code, this is written as if OpenGL 2 is the newest greatest thing, because I'm trapped in a dimension where it _is_.

Using these in a project other than mine will require some modifications, and some bending over backward. Probably not worth it except for the above components.
