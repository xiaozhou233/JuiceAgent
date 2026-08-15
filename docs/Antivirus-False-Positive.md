# 杀毒软件误报说明 / Antivirus False Positive Notice

## 中文

JuiceAgent 的 `libloader.dll` 在部分杀毒软件中可能被误报为恶意程序。原因如下：

1. **进程注入技术**：本项目需通过远程线程将 DLL 注入到目标 JVM 进程（`VirtualAllocEx` + `WriteProcessMemory` + `CreateRemoteThread`），这是杀软对"进程注入"行为的常见告警特征。
2. **反射加载（Reflective Loading）**：`libloader.dll` 使用反射加载器在内存中手动解析 PE 并加载，不经过常规的 `LoadLibrary` 加载流程，与部分恶意软件（无文件攻击）的技术高度相似。
3. **API 哈希**：为规避导入表，代码使用哈希解析 `LoadLibrary`/`GetProcAddress` 等 API，该手法也是杀软重点检测的静态特征。

以上技术均为实现 JVMTI 运行时注入所必需，项目本身不包含任何恶意行为。由于这些行为与恶意软件特征高度重合，**无法完全消除误报**。请在使用前自行判断风险，或将其加入杀软白名单。

## English

`libloader.dll` in JuiceAgent may be flagged as malware by some antivirus software. The reasons are as follows:

1. **Process Injection**: This project injects a DLL into the target JVM process via a remote thread (`VirtualAllocEx` + `WriteProcessMemory` + `CreateRemoteThread`). Process injection is a common behavior signature that antivirus engines flag.
2. **Reflective Loading**: `libloader.dll` uses a reflective loader to manually parse and map a PE image in memory, bypassing the normal `LoadLibrary` loading flow. This closely resembles fileless malware techniques.
3. **API Hashing**: The code resolves APIs such as `LoadLibrary`/`GetProcAddress` by hash instead of using an import table, a static feature also heavily targeted by AV engines.

These techniques are all required to implement JVMTI runtime injection. The project itself contains no malicious behavior. However, because these behaviors closely overlap with malware signatures, **false positives cannot be completely eliminated**. Please assess the risk before use, or add the binaries to your antivirus whitelist.
