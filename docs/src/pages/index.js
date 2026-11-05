import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';
import styles from './index.module.css';

export default function Home() {
  return (
    <Layout
      title="Bungalow"
      description="An open, plugin operating system for music, built on the Space UI framework">
      <main className={styles.main}>
        <section className={styles.hero}>
          <p className={styles.eyebrow}>A music and media manager</p>
          <h1>Bungalow</h1>
          <p className={styles.tagline}>
            An open, plugin operating system for music, built on the Space UI
            framework.
          </p>
          <div className={styles.actions}>
            <Link className="button button--primary button--lg" to="/docs/">
              Explore the documentation
            </Link>
            <Link
              className="button button--secondary button--lg"
              href="https://github.com/drsounds/bungalow-client">
              View on GitHub
            </Link>
          </div>
        </section>
      </main>
    </Layout>
  );
}
