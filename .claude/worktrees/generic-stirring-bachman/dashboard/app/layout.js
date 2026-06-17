import './globals.css';

export const metadata = {
  title: 'VTIC Election Dashboard',
  description: 'Smart School Election Terminal — Admin Dashboard',
};

export default function RootLayout({ children }) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
