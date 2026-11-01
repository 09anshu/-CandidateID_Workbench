import type { Config } from "tailwindcss";

const config: Config = {
  content: [
    "./pages/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  darkMode: "class",
  theme: {
    extend: {
      colors: {
        workbench: {
          dark: "#0b0f19",
          card: "#111827",
          border: "#1f2937",
          accent: "#2563eb",
          accentHover: "#1d4ed8",
          text: "#f9fafb",
          muted: "#9ca3af",
          success: "#10b981",
          warning: "#f59e0b",
          error: "#ef4444",
        },
      },
    },
  },
  plugins: [],
};
export default config;

