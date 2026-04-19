
#ifndef RHID3D12_API_H
#define RHID3D12_API_H

#ifdef RHID3D12_STATIC
#  define RHID3D12_API
#  define RHID3D12_NO_EXPORT
#else
#  ifndef RHID3D12_API
#    ifdef RHID3D12_EXPORTS
        /* We are building this library */
#      define RHID3D12_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define RHID3D12_API __declspec(dllimport)
#    endif
#  endif

#  ifndef RHID3D12_NO_EXPORT
#    define RHID3D12_NO_EXPORT 
#  endif
#endif

#ifndef RHID3D12_DEPRECATED
#  define RHID3D12_DEPRECATED __declspec(deprecated)
#endif

#ifndef RHID3D12_DEPRECATED_EXPORT
#  define RHID3D12_DEPRECATED_EXPORT RHID3D12_API RHID3D12_DEPRECATED
#endif

#ifndef RHID3D12_DEPRECATED_NO_EXPORT
#  define RHID3D12_DEPRECATED_NO_EXPORT RHID3D12_NO_EXPORT RHID3D12_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef RHID3D12_NO_DEPRECATED
#    define RHID3D12_NO_DEPRECATED
#  endif
#endif

#endif /* RHID3D12_API_H */
