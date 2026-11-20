import type {Metadata} from 'next';
import { JetBrains_Mono } from 'next/font/google';
import './globals.css'; // Global styles

const jetbrainsMono = JetBrains_Mono({
  subsets: ['latin'],
  variable: '--font-mono',
});

export const metadata: Metadata = {
  title: 'Terminal ADIF Monitor',
  description: 'ADIF Logger and monitor in a terminal UI',
};

export default function RootLayout({children}: {children: React.ReactNode}) {
  return (
    <html lang="en" className={`${jetbrainsMono.variable}`}>
      <body suppressHydrationWarning className="bg-black text-green-500 font-mono antialiased min-h-screen">
        {children}
      </body>
    </html>
  );
}
