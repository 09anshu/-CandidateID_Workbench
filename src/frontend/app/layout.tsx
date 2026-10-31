import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "PASS SCIENCE WORKS — Unified Engineering Workbench",
  description: "C++17 Numerical Engineering Workbench & Scientific Computing Suite",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark">
      <body className="bg-slate-950 text-slate-100 antialiased min-h-screen">
        {children}
      </body>
    </html>
  );
}
