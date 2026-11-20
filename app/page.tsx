import fs from 'fs/promises';
import path from 'path';
import crypto from 'crypto';

export default async function Page() {
  const mcPath = path.join(process.cwd(), 'cpp_source', 'mainwindow.cpp');
  let mc = '';
  try {
    mc = await fs.readFile(mcPath, 'utf8');
  } catch (e) {
    mc = 'Error reading mainwindow.cpp';
  }
  
  // Calculate md5 checksum
  const hash = crypto.createHash('md5').update(mc).digest('hex');

  // Extract menu structure
  const menuLines = mc.split('\n').filter(line => 
    line.includes('addMenuBtn') || 
    line.includes('addActionBtn') || 
    line.includes('windowsMenu->addAction') ||
    line.includes('m_trackingMenu->addAction') ||
    line.includes('configMenu->addAction') ||
    line.includes('configMenu->addMenu') ||
    line.includes('miscMenu->addAction') ||
    line.includes('aboutMenu->addAction')
  ).join('\n');

  return (
    <div className="min-h-screen p-8 bg-gray-50 text-gray-900 font-mono text-sm leading-relaxed max-w-7xl mx-auto">
      <h1 className="text-2xl font-bold mb-8">ADIF Monitor Current Source State</h1>
      
      <div className="mb-4 p-4 bg-green-100 rounded-lg shadow border border-green-300">
        <h2 className="text-xl font-bold text-green-900 mb-2">QSL Manager UX & Sorting Refactor</h2>
        <ul className="text-lg font-sans space-y-2 list-disc list-inside">
          <li><strong>Persistent State Memory:</strong> Opening the QSL Manager now instantly recreates the exact last search payload you performed, restoring any active date ranges and preserving continuity between workspace operations.</li>
          <li><strong>Modern UI Theme:</strong> Refactored QSL buttons and sorting widgets applying a modern visual design, technical monospace typefaces, and custom sleek checkbox overlays replacing native components.</li>
          <li><strong>Quick Filter Bar:</strong> Appended rapid temporal scoping options directly into the primary command rail: <code>Show Last X Days</code> (dynamic dialog), <code>Past Week</code>, and <code>Today</code> — instantly refining search results without manual calendar inputs.</li>
          <li><strong>Data Alignment Craft:</strong> All column contents dynamically flow with precision; target callsigns firmly anchor to left alignment, with all metrics, bands, flags, and modes elegantly aligned and centered globally.</li>
        </ul>
      </div>

      <div className="mb-4 p-4 bg-blue-100 rounded-lg shadow border border-blue-300">
        <h2 className="text-xl font-bold text-blue-900 mb-2">Build Verification Checksum</h2>
        <p className="text-lg font-mono">MD5 of mainwindow.cpp: <span className="font-bold text-blue-700">{hash}</span></p>
        <p className="text-sm mt-2 text-blue-800">Use this checksum locally to verify your export correctly fetched the latest changes.</p>
      </div>

      <div className="space-y-12">
        <section>
          <h2 className="text-xl font-black mb-4 bg-gray-200 p-2 inline-block shadow">Menu Generation Lines in Code</h2>
          <pre className="bg-white p-4 rounded-xl shadow border border-gray-300 overflow-x-auto text-xs font-mono">
            {menuLines}
          </pre>
        </section>
      </div>
    </div>
  );
}
