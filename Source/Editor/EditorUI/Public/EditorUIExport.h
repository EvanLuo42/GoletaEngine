
#ifndef EDITORUI_API_H
#define EDITORUI_API_H

#ifdef EDITORUI_STATIC
#  define EDITORUI_API
#  define EDITORUI_NO_EXPORT
#else
#  ifndef EDITORUI_API
#    ifdef EditorUI_EXPORTS
        /* We are building this library */
#      define EDITORUI_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define EDITORUI_API __declspec(dllimport)
#    endif
#  endif

#  ifndef EDITORUI_NO_EXPORT
#    define EDITORUI_NO_EXPORT 
#  endif
#endif

#ifndef EDITORUI_DEPRECATED
#  define EDITORUI_DEPRECATED __declspec(deprecated)
#endif

#ifndef EDITORUI_DEPRECATED_EXPORT
#  define EDITORUI_DEPRECATED_EXPORT EDITORUI_API EDITORUI_DEPRECATED
#endif

#ifndef EDITORUI_DEPRECATED_NO_EXPORT
#  define EDITORUI_DEPRECATED_NO_EXPORT EDITORUI_NO_EXPORT EDITORUI_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef EDITORUI_NO_DEPRECATED
#    define EDITORUI_NO_DEPRECATED
#  endif
#endif

#endif /* EDITORUI_API_H */
