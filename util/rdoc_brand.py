import os
from urllib.parse import quote


def _getenv(name: str, default: str) -> str:
    return os.environ.get(name, default)


def _mailto_url(subject: str) -> str:
    return f"mailto:{SUPPORT_EMAIL}?subject={quote(subject)}"


PRODUCT_NAME = _getenv("RDOC_PRODUCT_NAME", "RenderDoc")
BASE_NAME = _getenv("RDOC_BASE_NAME", "renderdoc")
MANUFACTURER_NAME = _getenv("RDOC_MANUFACTURER_NAME", "Baldur Karlsson")
UI_NAME = _getenv("RDOC_UI_NAME", "qrenderdoc")
UI_DISPLAY_NAME = _getenv("RDOC_UI_DISPLAY_NAME", "QRenderDoc")
UI_EXECUTABLE = _getenv("RDOC_UI_EXECUTABLE", "qrenderdoc.exe")
CMD_NAME = _getenv("RDOC_CMD_NAME", "renderdoccmd")
CMD_DISPLAY_NAME = _getenv("RDOC_CMD_DISPLAY_NAME", "RenderDocCmd")
CMD_EXECUTABLE = _getenv("RDOC_CMD_EXECUTABLE", "renderdoccmd.exe")
PY_CORE_MODULE_NAME = _getenv("RDOC_PY_CORE_MODULE_NAME", "renderdoc")
PY_GUI_MODULE_NAME = _getenv("RDOC_PY_GUI_MODULE_NAME", "qrenderdoc")
CORE_DLL_NAME = _getenv("RDOC_CORE_DLL_NAME", "renderdoc.dll")
LINUX_CORE_LIBRARY = _getenv("RDOC_LINUX_CORE_LIBRARY", "librenderdoc.so")
APPLE_CORE_LIBRARY = _getenv("RDOC_APPLE_CORE_LIBRARY", "librenderdoc.dylib")
ANDROID_CAPTURE_LIBRARY = _getenv("RDOC_ANDROID_CAPTURE_LIBRARY", "libVkLayer_GLES_RenderDoc.so")
WEBSITE_URL = _getenv("RDOC_WEBSITE_URL", "https://renderdoc.org")
DOCUMENTATION_URL = _getenv("RDOC_DOCUMENTATION_URL", WEBSITE_URL + "/docs")
PYTHON_API_URL = _getenv("RDOC_PYTHON_API_URL", DOCUMENTATION_URL + "/python_api/index.html")
IN_APPLICATION_API_URL = _getenv("RDOC_IN_APPLICATION_API_URL", DOCUMENTATION_URL + "/in_application_api.html")
BUILDS_URL = _getenv("RDOC_BUILDS_URL", WEBSITE_URL + "/builds")
UPDATE_URL_PATTERN = _getenv(
    "RDOC_UPDATE_URL_PATTERN",
    WEBSITE_URL + "/getupdateurl/{bitness}/{version}?htmlnotes=1",
)
TIPS_URL_PATTERN = _getenv("RDOC_TIPS_URL_PATTERN", WEBSITE_URL + "/tips/{index}")
BUGREPORT_URL = _getenv("RDOC_BUGREPORT_URL", WEBSITE_URL + "/bugreporter")
ANALYTICS_URL = _getenv("RDOC_ANALYTICS_URL", WEBSITE_URL + "/analytics")
SUPPORT_EMAIL = _getenv("RDOC_SUPPORT_EMAIL", "baldurk@baldurk.org")
SOURCE_REPO_USER = _getenv("RDOC_SOURCE_REPO_USER", "baldurk")
SOURCE_REPO_NAME = _getenv("RDOC_SOURCE_REPO_NAME", "renderdoc")
SOURCE_URL = _getenv("RDOC_SOURCE_URL", f"https://github.com/{SOURCE_REPO_USER}/{SOURCE_REPO_NAME}")
ISSUES_URL = _getenv("RDOC_ISSUES_URL", SOURCE_URL + "/issues")
CONTRIB_URL = _getenv("RDOC_CONTRIB_URL", "https://github.com/baldurk/renderdoc-contrib")
SUPPORT_BUG_URL = _getenv("RDOC_SUPPORT_BUG_URL", _mailto_url(f"{PRODUCT_NAME} bug"))
SUPPORT_FEEDBACK_URL = _getenv("RDOC_SUPPORT_FEEDBACK_URL", _mailto_url(f"{PRODUCT_NAME} feedback"))
SUPPORT_QUESTION_URL = _getenv("RDOC_SUPPORT_QUESTION_URL", _mailto_url(f"{PRODUCT_NAME} question"))
SUPPORT_UNRECOVERABLE_URL = _getenv(
    "RDOC_SUPPORT_UNRECOVERABLE_URL",
    _mailto_url(f"{PRODUCT_NAME} Unrecoverable error"),
)
CMD_ANDROID_PACKAGE_BASE = _getenv(
    "RDOC_CMD_ANDROID_PACKAGE_BASE",
    "org.renderdoc.renderdoccmd",
)
TEST_ANDROID_DEMO_PACKAGE_BASE = _getenv(
    "RDOC_TEST_ANDROID_DEMO_PACKAGE_BASE",
    "renderdoc.org.demos",
)
