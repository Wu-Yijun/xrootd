import * as fs from 'fs';
import * as path from 'path';

interface GenerateOptions {
  size: number;           // 文件大小（字节）
  outputPath: string;     // 输出文件路径
  binary?: boolean;       // 是否生成二进制文件（false则生成文本文件）
  chunkSize?: number;     // 写入块大小（默认64KB）
}

/**
 * 生成随机字节缓冲区
 */
function generateRandomBuffer(size: number, binary: boolean = true): Buffer {
  const buffer = Buffer.allocUnsafe(size);
  
  if (binary) {
    // 填充随机字节（0-255）
    for (let i = 0; i < size; i++) {
      buffer[i] = Math.floor(Math.random() * 256);
    }
  } else {
    // 生成可打印ASCII字符（32-126）
    for (let i = 0; i < size; i++) {
      // 65-122 包含大小写字母，避免特殊字符
      buffer[i] = 65 + Math.floor(Math.random() * 58);
    }
  }
  
  return buffer;
}

/**
 * 解析大小字符串（如 "1MB", "500KB", "10GB"）
 */
function parseSize(sizeStr: string): number {
  const units: { [key: string]: number } = {
    'B': 1,
    'KB': 1024,
    'MB': 1024 * 1024,
    'GB': 1024 * 1024 * 1024,
    'TB': 1024 * 1024 * 1024 * 1024
  };
  
  const match = sizeStr.match(/^(\d+(?:\.\d+)?)\s*(B|KB|MB|GB|TB)?$/i);
  if (!match) {
    throw new Error(`Invalid size format: ${sizeStr}`);
  }
  
  const value = parseFloat(match[1]);
  const unit = match[2] ? match[2].toUpperCase() : 'B';
  
  return Math.floor(value * units[unit]);
}

/**
 * 格式化文件大小显示
 */
function formatSize(bytes: number): string {
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  let size = bytes;
  let unitIndex = 0;
  
  while (size >= 1024 && unitIndex < units.length - 1) {
    size /= 1024;
    unitIndex++;
  }
  
  return `${size.toFixed(2)} ${units[unitIndex]}`;
}

/**
 * 生成随机文件
 */
async function generateRandomFile(options: GenerateOptions): Promise<void> {
  const {
    size,
    outputPath,
    binary = true,
    chunkSize = 64 * 1024 // 默认64KB块
  } = options;
  
  // 确保输出目录存在
  const outputDir = path.dirname(outputPath);
  if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
  }
  
  console.log(`生成${binary ? '二进制' : '文本'}文件:`);
  console.log(`  路径: ${outputPath}`);
  console.log(`  大小: ${formatSize(size)}`);
  console.log(`  块大小: ${formatSize(chunkSize)}`);
  
  const writeStream = fs.createWriteStream(outputPath);
  let written = 0;
  let startTime = Date.now();
  
  return new Promise((resolve, reject) => {
    const writeChunk = () => {
      const remaining = size - written;
      if (remaining <= 0) {
        writeStream.end();
        const duration = (Date.now() - startTime) / 1000;
        console.log(`\n✅ 文件生成完成！`);
        console.log(`   耗时: ${duration.toFixed(2)} 秒`);
        console.log(`   速度: ${formatSize(size / duration)}/秒`);
        resolve();
        return;
      }
      
      const currentChunkSize = Math.min(chunkSize, remaining);
      const chunk = generateRandomBuffer(currentChunkSize, binary);
      
      const canWrite = writeStream.write(chunk);
      written += currentChunkSize;
      
      // 显示进度
      if (written % (chunkSize * 10) === 0 || written === size) {
        const percent = (written / size * 100).toFixed(1);
        process.stdout.write(`\r进度: ${percent}% (${formatSize(written)}/${formatSize(size)})`);
      }
      
      if (canWrite) {
        writeChunk();
      } else {
        writeStream.once('drain', writeChunk);
      }
    };
    
    writeStream.on('error', reject);
    writeStream.on('finish', () => {
      // 进度已显示
    });
    
    writeChunk();
  });
}

/**
 * 命令行接口
 */
async function main() {
  const args = process.argv.slice(2);
  
  if (args.length < 2 || args.includes('--help') || args.includes('-h')) {
    console.log(`
使用方法:
  node generate-random-file.ts <大小> <输出路径> [选项]

参数:
  大小        文件大小，支持格式: 1024, 1KB, 5MB, 2GB, 1.5TB
  输出路径    目标文件路径

选项:
  --text      生成文本文件（默认为二进制）
  --chunk-size <大小>  写入块大小（默认64KB）
  --help, -h  显示帮助信息

示例:
  # 生成1MB的二进制文件
  node generate-random-file.ts 1MB output.bin

  # 生成500KB的文本文件
  node generate-random-file.ts 500KB output.txt --text

  # 生成2GB文件，使用1MB块大小
  node generate-random-file.ts 2GB large.bin --chunk-size 1MB
`);
    return;
  }
  
  try {
    const sizeStr = args[0];
    const outputPath = args[1];
    const isText = args.includes('--text');
    
    // 解析块大小
    let chunkSize = 64 * 1024;
    const chunkSizeIndex = args.indexOf('--chunk-size');
    if (chunkSizeIndex !== -1 && args[chunkSizeIndex + 1]) {
      chunkSize = parseSize(args[chunkSizeIndex + 1]);
    }
    
    const size = parseSize(sizeStr);
    
    await generateRandomFile({
      size,
      outputPath,
      binary: !isText,
      chunkSize
    });
  } catch (error) {
    console.error('❌ 错误:', error instanceof Error ? error.message : error);
    process.exit(1);
  }
}

if(import.meta.main){
  await main();
}

export { generateRandomFile, parseSize, formatSize, type GenerateOptions };