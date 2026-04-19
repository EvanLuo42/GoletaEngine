
#ifndef RENDERGRAPH_API_H
#define RENDERGRAPH_API_H

#ifdef RENDERGRAPH_STATIC
#  define RENDERGRAPH_API
#  define RENDERGRAPH_NO_EXPORT
#else
#  ifndef RENDERGRAPH_API
#    ifdef RenderGraph_EXPORTS
        /* We are building this library */
#      define RENDERGRAPH_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define RENDERGRAPH_API __declspec(dllimport)
#    endif
#  endif

#  ifndef RENDERGRAPH_NO_EXPORT
#    define RENDERGRAPH_NO_EXPORT 
#  endif
#endif

#ifndef RENDERGRAPH_DEPRECATED
#  define RENDERGRAPH_DEPRECATED __declspec(deprecated)
#endif

#ifndef RENDERGRAPH_DEPRECATED_EXPORT
#  define RENDERGRAPH_DEPRECATED_EXPORT RENDERGRAPH_API RENDERGRAPH_DEPRECATED
#endif

#ifndef RENDERGRAPH_DEPRECATED_NO_EXPORT
#  define RENDERGRAPH_DEPRECATED_NO_EXPORT RENDERGRAPH_NO_EXPORT RENDERGRAPH_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef RENDERGRAPH_NO_DEPRECATED
#    define RENDERGRAPH_NO_DEPRECATED
#  endif
#endif

#endif /* RENDERGRAPH_API_H */
