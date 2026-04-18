
#ifndef EDITORCORE_API_H
#define EDITORCORE_API_H

#ifdef EDITORCORE_STATIC
#  define EDITORCORE_API
#  define EDITORCORE_NO_EXPORT
#else
#  ifndef EDITORCORE_API
#    ifdef EditorCore_EXPORTS
        /* We are building this library */
#      define EDITORCORE_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define EDITORCORE_API __declspec(dllimport)
#    endif
#  endif

#  ifndef EDITORCORE_NO_EXPORT
#    define EDITORCORE_NO_EXPORT 
#  endif
#endif

#ifndef EDITORCORE_DEPRECATED
#  define EDITORCORE_DEPRECATED __declspec(deprecated)
#endif

#ifndef EDITORCORE_DEPRECATED_EXPORT
#  define EDITORCORE_DEPRECATED_EXPORT EDITORCORE_API EDITORCORE_DEPRECATED
#endif

#ifndef EDITORCORE_DEPRECATED_NO_EXPORT
#  define EDITORCORE_DEPRECATED_NO_EXPORT EDITORCORE_NO_EXPORT EDITORCORE_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef EDITORCORE_NO_DEPRECATED
#    define EDITORCORE_NO_DEPRECATED
#  endif
#endif

#endif /* EDITORCORE_API_H */
