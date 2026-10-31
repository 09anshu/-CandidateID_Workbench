"use client";

import React, { useState } from "react";

export interface DataSeries {
  name: string;
  color: string;
  points: Array<{ x: number; y: number }>;
  dashed?: boolean;
}

interface ScientificChartProps {
  title: string;
  xLabel: string;
  yLabel: string;
  series: DataSeries[];
  logX?: boolean;
  logY?: boolean;
  height?: number;
}

export default function ScientificChart({
  title,
  xLabel,
  yLabel,
  series,
  logX = false,
  logY = false,
  height = 320,
}: ScientificChartProps) {
  const [hoveredPoint, setHoveredPoint] = useState<{
    x: number;
    y: number;
    seriesName: string;
    screenX: number;
    screenY: number;
  } | null>(null);

  // Compute bounding box
  let minX = Infinity;
  let maxX = -Infinity;
  let minY = Infinity;
  let maxY = -Infinity;

  series.forEach((s) => {
    s.points.forEach((p) => {
      const px = logX ? (p.x > 0 ? Math.log10(p.x) : 0) : p.x;
      const py = logY ? (p.y > 0 ? Math.log10(p.y) : 0) : p.y;
      if (px < minX) minX = px;
      if (px > maxX) maxX = px;
      if (py < minY) minY = py;
      if (py > maxY) maxY = py;
    });
  });

  if (!isFinite(minX)) {
    minX = 0; maxX = 1;
    minY = 0; maxY = 1;
  }

  // Padding
  const xSpan = Math.max(maxX - minX, 1e-4);
  const ySpan = Math.max(maxY - minY, 1e-4);
  const plotMinX = minX;
  const plotMaxX = maxX;
  const plotMinY = Math.max(0, minY - ySpan * 0.05);
  const plotMaxY = maxY + ySpan * 0.05;

  const padLeft = 65;
  const padRight = 25;
  const padTop = 35;
  const padBottom = 45;
  const width = 640;

  const toScreenX = (xVal: number) => {
    const val = logX ? (xVal > 0 ? Math.log10(xVal) : 0) : xVal;
    return padLeft + ((val - plotMinX) / (plotMaxX - plotMinX || 1)) * (width - padLeft - padRight);
  };

  const toScreenY = (yVal: number) => {
    const val = logY ? (yVal > 0 ? Math.log10(yVal) : 0) : yVal;
    return height - padBottom - ((val - plotMinY) / (plotMaxY - plotMinY || 1)) * (height - padTop - padBottom);
  };

  // Generate grid ticks
  const xTicks = 5;
  const yTicks = 5;
  const xTickValues = Array.from({ length: xTicks + 1 }, (_, i) => plotMinX + (i / xTicks) * (plotMaxX - plotMinX));
  const yTickValues = Array.from({ length: yTicks + 1 }, (_, i) => plotMinY + (i / yTicks) * (plotMaxY - plotMinY));

  return (
    <div className="relative w-full bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-xl select-none">
      <div className="flex items-center justify-between mb-2">
        <h3 className="text-sm font-semibold text-slate-200 tracking-wide">{title}</h3>
        {/* Legend */}
        <div className="flex items-center space-x-3 text-xs">
          {series.map((s) => (
            <div key={s.name} className="flex items-center space-x-1.5">
              <span
                className="w-3 h-0.5 inline-block"
                style={{
                  backgroundColor: s.color,
                  borderTop: s.dashed ? `2px dashed ${s.color}` : `2px solid ${s.color}`,
                }}
              />
              <span className="text-slate-300">{s.name}</span>
            </div>
          ))}
        </div>
      </div>

      <div className="relative w-full overflow-hidden">
        <svg
          viewBox={`0 0 ${width} ${height}`}
          className="w-full h-auto"
          onMouseLeave={() => setHoveredPoint(null)}
        >
          {/* Background grid */}
          {xTickValues.map((tx, idx) => {
            const sx = toScreenX(logX ? Math.pow(10, tx) : tx);
            return (
              <g key={`xtick-${idx}`}>
                <line x1={sx} y1={padTop} x2={sx} y2={height - padBottom} stroke="#1e293b" strokeDasharray="3 3" />
                <text x={sx} y={height - padBottom + 16} fill="#64748b" fontSize="10" textAnchor="middle">
                  {logX ? `10^${tx.toFixed(1)}` : tx.toFixed(2)}
                </text>
              </g>
            );
          })}

          {yTickValues.map((ty, idx) => {
            const sy = toScreenY(logY ? Math.pow(10, ty) : ty);
            return (
              <g key={`ytick-${idx}`}>
                <line x1={padLeft} y1={sy} x2={width - padRight} y2={sy} stroke="#1e293b" strokeDasharray="3 3" />
                <text x={padLeft - 8} y={sy + 3} fill="#64748b" fontSize="10" textAnchor="end">
                  {logY ? `10^${ty.toFixed(1)}` : ty >= 100 ? ty.toFixed(0) : ty.toFixed(2)}
                </text>
              </g>
            );
          })}

          {/* Axes */}
          <line x1={padLeft} y1={height - padBottom} x2={width - padRight} y2={height - padBottom} stroke="#475569" strokeWidth="1.5" />
          <line x1={padLeft} y1={padTop} x2={padLeft} y2={height - padBottom} stroke="#475569" strokeWidth="1.5" />

          {/* Axis Labels */}
          <text x={(width + padLeft - padRight) / 2} y={height - 8} fill="#94a3b8" fontSize="11" textAnchor="middle" fontWeight="500">
            {xLabel}
          </text>
          <text
            x={-((height + padTop - padBottom) / 2)}
            y={16}
            fill="#94a3b8"
            fontSize="11"
            textAnchor="middle"
            transform="rotate(-90)"
            fontWeight="500"
          >
            {yLabel}
          </text>

          {/* Data Lines */}
          {series.map((s) => {
            if (s.points.length === 0) return null;
            const pathData = s.points
              .map((p, i) => `${i === 0 ? "M" : "L"} ${toScreenX(p.x)} ${toScreenY(p.y)}`)
              .join(" ");

            return (
              <g key={s.name}>
                <path
                  d={pathData}
                  fill="none"
                  stroke={s.color}
                  strokeWidth="2.2"
                  strokeDasharray={s.dashed ? "4 4" : undefined}
                />
                {/* Points */}
                {s.points.map((p, i) => {
                  const sx = toScreenX(p.x);
                  const sy = toScreenY(p.y);
                  return (
                    <circle
                      key={i}
                      cx={sx}
                      cy={sy}
                      r="3.5"
                      fill={s.color}
                      stroke="#0f172a"
                      strokeWidth="1.5"
                      className="cursor-pointer hover:r-5 transition-all"
                      onMouseEnter={() =>
                        setHoveredPoint({
                          x: p.x,
                          y: p.y,
                          seriesName: s.name,
                          screenX: sx,
                          screenY: sy,
                        })
                      }
                    />
                  );
                })}
              </g>
            );
          })}
        </svg>

        {/* Hover Tooltip */}
        {hoveredPoint && (
          <div
            className="absolute pointer-events-none bg-slate-800/95 text-slate-100 text-xs px-2.5 py-1.5 rounded border border-slate-600 shadow-xl"
            style={{
              left: `${(hoveredPoint.screenX / width) * 100}%`,
              top: `${(hoveredPoint.screenY / height) * 100}%`,
              transform: "translate(-50%, -125%)",
            }}
          >
            <div className="font-semibold text-blue-400">{hoveredPoint.seriesName}</div>
            <div>X: {hoveredPoint.x.toFixed(4)}</div>
            <div>Y: {hoveredPoint.y.toFixed(4)}</div>
          </div>
        )}
      </div>
    </div>
  );
}
