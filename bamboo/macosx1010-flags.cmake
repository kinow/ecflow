# On mac appears no static python boost lib, hence use shared for everything
SET( ENABLE_STATIC_BOOST_LIBS   OFF CACHE BOOL "Use dynamic boost libs")
SET( ENABLE_PYTHON_PTR_REGISTER  ON CACHE BOOL "Register shared ptr to python converters")

