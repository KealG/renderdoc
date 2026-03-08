import importlib

import rdoc_brand as brand

rd = importlib.import_module(brand.PY_CORE_MODULE_NAME)
qrd = importlib.import_module(brand.PY_GUI_MODULE_NAME)

__all__ = ["brand", "rd", "qrd"]
