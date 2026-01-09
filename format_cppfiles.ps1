<#
.SYNOPSIS
  对所有 C/C++ 文件批量格式化并保存（显式使用指定的 clang-format.exe），排除 build 目录。

.DESCRIPTION
  1. 自动查找 clang-format.exe 路径
  2. 递归查找当前目录及子目录下的所有 .c/.cpp/.h/.hpp 文件
  3. 排除任何路径中包含 \build\ 或 /build/ 的文件
  4. 对其余文件执行 clang-format -i 原地格式化

.NOTES
  • 将本脚本放在项目根目录后运行：
      PS> .\Format-UnmodifiedCpp.ps1
#>

# 1. 切到脚本所在目录（项目根）
Param(
    [string[]]$ExcludeDirs = @()
)

# 切到脚本所在目录（项目根）
Set-Location (Split-Path -Parent $MyInvocation.MyCommand.Path)

# 可在脚本内指定要排除的文件夹（编辑此处以固定排除某些目录），
# 命令行通过 `-ExcludeDirs` 仍然可以附加或覆盖（合并后去重）。
# 例如：@('3rdparty','out','vendor')
$ScriptExcludeDirs = @('3rdparty')

# 2. 自动查找 clang-format.exe 路径
$clangFormatPath = $null

# 2.1 查找环境变量（PATH）
$clangFormatPath = Get-Command clang-format.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1

# 2.2 查找 VSCode 插件目录
if (-not $clangFormatPath) {
    $vsCodePath = "$env:USERPROFILE\.vscode\extensions"
    if (Test-Path $vsCodePath) {
        $clangFormatPath = Get-ChildItem -Path $vsCodePath -Recurse -Filter clang-format.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName -First 1
    }
}

# 2.3 查找 LLVM 官方安装路径
if (-not $clangFormatPath) {
    $llvmPath = "C:\Program Files\LLVM\bin\clang-format.exe"
    if (Test-Path $llvmPath) {
        $clangFormatPath = $llvmPath
    }
}

# 2.4 兜底：手动指定
if (-not $clangFormatPath) {
    $clangFormatPath = 'C:\Users\kk\.vscode\extensions\ms-vscode.cpptools-1.26.3-win32-x64\LLVM\bin\clang-format.exe'
}

# 3. 检查 clang-format.exe 是否存在
if (-not (Test-Path $clangFormatPath)) {
    Write-Error "❗ 未能自动找到 clang-format.exe，请手动指定路径。当前尝试路径：$clangFormatPath"
    exit 1
}

Write-Host "✔ 使用 clang-format 路径：$clangFormatPath" -ForegroundColor Green

# 4. 查找所有 C/C++ 文件，排除指定文件夹（默认包含 build）
# 处理排除列表并构造正则表达式以匹配路径中的目录分隔符
$defaultExcludes = @('build')

# 合并脚本内配置、命令行传入和默认排除项，去空并去重
$allProvided = @()
if ($ScriptExcludeDirs) { $allProvided += $ScriptExcludeDirs }
if ($ExcludeDirs) { $allProvided += $ExcludeDirs }

$excludeDirs = @($allProvided + $defaultExcludes) | Where-Object { $_ -and $_.Trim() -ne '' } | Select-Object -Unique

if ($excludeDirs.Count -gt 0) {
    $escaped = $excludeDirs | ForEach-Object { [regex]::Escape($_) }
    $patternParts = $escaped | ForEach-Object { "[\\/]+$_[\\/]" }
    $excludePattern = ($patternParts -join '|')
}

$files = Get-ChildItem -Recurse -File -Include '*.c', '*.cpp', '*.h', '*.hpp' |
Where-Object {
    if (-not $excludePattern) { return $true }
    -not ($_.FullName -match $excludePattern)
}

if (-not $files) {
    Write-Host "⚠️ 未找到任何符合条件的 C/C++ 源文件。" -ForegroundColor Yellow
    exit 0
}

$scriptList = if ($ScriptExcludeDirs -and $ScriptExcludeDirs.Count -gt 0) { $ScriptExcludeDirs -join ', ' } else { '<none>' }
$cliList = if ($ExcludeDirs -and $ExcludeDirs.Count -gt 0) { $ExcludeDirs -join ', ' } else { '<none>' }
Write-Host "🔄 开始使用 `$clangFormatPath` 格式化以下文件（脚本排除：$scriptList；命令行排除：$cliList；合并后：$(($excludeDirs -join ', '))）" -ForegroundColor Cyan
$files | ForEach-Object { Write-Host "  $_.FullName" }

# 5. 循环格式化
foreach ($file in $files) {
    & $clangFormatPath -i $file.FullName
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ 格式化成功：$($file.Name)"
    }
    else {
        Write-Host "❌ 格式化失败：$($file.Name)" -ForegroundColor Red
    }
}

Write-Host "`n🎉 格式化完成！" -ForegroundColor Green
