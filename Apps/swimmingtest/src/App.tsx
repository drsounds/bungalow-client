import React, { useEffect, useState } from 'react';
import { BrowserRouter, Routes, Route, useLocation, useNavigate } from 'react-router-dom';

// Bungalow delivers bungalow:app:swimmingtest:a:b as the route /a/b
// (history.replaceState plus popstate), which BrowserRouter follows.

const page = { padding: 24, backgroundColor: '#1e293b', flexGrow: 1, flexDirection: 'column' as const, gap: 12 };
const heading = { color: '#38bdf8', fontSize: 22 };
const text = { color: '#cbd5e1' };
const button = {
  backgroundColor: '#0284c7', color: '#ffffff', padding: 8, borderRadius: 6, width: 180, height: 32,
};

function Clock() {
  const [seconds, setSeconds] = useState(0);
  useEffect(() => {
    const timer = setInterval(() => setSeconds((s) => s + 1), 1000);
    return () => clearInterval(timer);
  }, []);
  return <p style={text}>Running for {seconds} s (JS timers)</p>;
}

function Home() {
  const [clicks, setClicks] = useState(0);
  const navigate = useNavigate();
  return (
    <div style={page}>
      <h1 style={heading}>Swimming Test</h1>
      <p style={text}>A React app rendered natively in Bungalow, without a browser engine.</p>
      <Clock />
      <button style={button} onClick={() => setClicks((c) => c + 1)}>
        Clicked {clicks} times
      </button>
      <button style={button} onClick={() => navigate('/about')}>
        Go to About
      </button>
    </div>
  );
}

function About() {
  const navigate = useNavigate();
  return (
    <div style={page}>
      <h1 style={heading}>About</h1>
      <p style={text}>Open bungalow:app:swimmingtest:about to land here directly.</p>
      <button style={button} onClick={() => navigate('/')}>
        Back to Home
      </button>
    </div>
  );
}

function NotFound() {
  const location = useLocation();
  return (
    <div style={page}>
      <h1 style={heading}>Not found</h1>
      <p style={text}>No page for {location.pathname}</p>
    </div>
  );
}

export function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<Home />} />
        <Route path="/about" element={<About />} />
        <Route path="*" element={<NotFound />} />
      </Routes>
    </BrowserRouter>
  );
}
