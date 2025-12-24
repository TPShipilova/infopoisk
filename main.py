#!/usr/bin/env python3

import requests
import time
import json
import re
import logging
import hashlib
import random
import pickle
import os
import sys
import ssl
from datetime import datetime, timedelta
from urllib.parse import urljoin, urlparse, urlunparse
from bs4 import BeautifulSoup
from pymongo import MongoClient
import urllib3
import wikipediaapi
from typing import List, Dict, Optional, Set, Tuple, Deque
from collections import deque

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

ssl._create_default_https_context = ssl._create_unverified_context

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('crawler.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)


class AdvancedFashionCrawler:
    """Продвинутый поисковый робот с рекурсивным обходом и сохранением состояния."""

    def __init__(self, mongo_uri="mongodb://localhost:27017/", state_file="crawler_state.pkl"):
        self.client = MongoClient(mongo_uri, serverSelectionTimeoutMS=10000)
        self.db = self.client["fashion_corpus"]

        # Коллекции
        self.documents = self.db["documents"]
        self.visited_urls = self.db["visited_urls"]
        self.crawler_state = self.db["crawler_state"]

        self._create_indexes()

        self.state_file = state_file

        self.crawl_delay = 3.0
        self.max_retries = 5
        self.batch_size = 50
        self.recheck_days = 30
        self.request_timeout = 45
        self.max_depth = 3

        self.headers_list = [
            {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
                'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
                'Accept-Language': 'en-US,en;q=0.5',
                'Accept-Encoding': 'gzip, deflate, br',
                'Connection': 'keep-alive',
                'Upgrade-Insecure-Requests': '1',
                'Cache-Control': 'max-age=0'
            },
            {
                'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.0 Safari/605.1.15',
                'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
                'Accept-Language': 'en-US,en;q=0.9',
                'Accept-Encoding': 'gzip, deflate, br'
            },
            {
                'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
                'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8',
                'Accept-Language': 'en-US,en;q=0.5'
            }
        ]

        self.source_configs = [
            {
                'name': 'Vogue',
                'base_url': 'https://www.vogue.com',
                'start_urls': [
                    'https://www.vogue.com/fashion',
                    'https://www.vogue.com/fashion/articles',
                    'https://www.vogue.com/runway',
                    'https://www.vogue.com/trends'
                ],
                'priority': 1,
                'article_patterns': ['/article/', '/story/', '/runway/', '/trends/']
            },
            {
                'name': 'Harper\'s Bazaar',
                'base_url': 'https://www.harpersbazaar.com',
                'start_urls': [
                    'https://www.harpersbazaar.com/fashion/',
                    'https://www.harpersbazaar.com/fashion/trends/',
                    'https://www.harpersbazaar.com/fashion/designers/'
                ],
                'priority': 1,
                'article_patterns': ['/fashion/', '/style/', '/trends/', '/designers/']
            },
            {
                'name': 'ELLE',
                'base_url': 'https://www.elle.com',
                'start_urls': [
                    'https://www.elle.com/fashion/',
                    'https://www.elle.com/trends/',
                    'https://www.elle.com/runway/'
                ],
                'priority': 1,
                'article_patterns': ['/fashion/', '/trends/', '/runway/', '/style/']
            },
            {
                'name': 'Business of Fashion',
                'base_url': 'https://www.businessoffashion.com',
                'start_urls': [
                    'https://www.businessoffashion.com/articles',
                    'https://www.businessoffashion.com/news',
                    'https://www.businessoffashion.com/analysis'
                ],
                'priority': 2,
                'article_patterns': ['/articles/', '/news/', '/analysis/']
            },
            {
                'name': 'The Fashion Law',
                'base_url': 'https://www.thefashionlaw.com',
                'start_urls': [
                    'https://www.thefashionlaw.com/category/fashion/',
                    'https://www.thefashionlaw.com/category/business/'
                ],
                'priority': 2,
                'article_patterns': ['/category/']
            }
        ]

        # Ключевые слова для фильтрации
        self.fashion_keywords = [
            'fashion', 'clothing', 'textile', 'designer', 'model', 'runway',
            'collection', 'couture', 'haute', 'tailoring', 'fabric', 'silk',
            'cotton', 'wool', 'linen', 'pattern', 'sewing', 'garment',
            'apparel', 'wardrobe', 'style', 'trend', 'vogue', 'mode',
            'dress', 'suit', 'jacket', 'skirt', 'blouse', 'accessory',
            'jewelry', 'footwear', 'handbag', 'perfume', 'cosmetics',
            'luxury', 'brand', 'retail', 'manufacturing', 'sustainable',
            'style', 'trendy', 'outfit', 'wardrobe', 'chic', 'elegant',
            'catwalk', 'fashion show', 'design', 'couturier', 'atelier'
        ]

        self.session = requests.Session()
        self.session.headers.update(self.headers_list[0])
        self.session.verify = False  # Отключаем проверку SSL для проблемных сайтов
        self.session.mount('https://', requests.adapters.HTTPAdapter(
            max_retries=3,
            pool_connections=10,
            pool_maxsize=10
        ))
        self.wiki_api = wikipediaapi.Wikipedia(
            language='en',
            user_agent='AdvancedFashionCrawler/1.0 (your_email@example.com)',
            extract_format=wikipediaapi.ExtractFormat.WIKI
        )

        self.crawler_status = {}
        self._load_state()

    def _create_indexes(self):
        """Создает необходимые индексы."""
        self.documents.create_index([("url", 1)], unique=True)
        self.documents.create_index([("content_hash", 1)])
        self.documents.create_index([("last_crawled", 1)])
        self.documents.create_index([("source", 1)])
        self.visited_urls.create_index([("url", 1)], unique=True)
        self.visited_urls.create_index([("domain", 1)])
        self.crawler_state.create_index([("source", 1)], unique=True)

    def _load_state(self):
        """Загружает состояние."""
        if os.path.exists(self.state_file):
            try:
                with open(self.state_file, 'rb') as f:
                    self.crawler_status = pickle.load(f)
                logger.info(f"Загружено состояние из {self.state_file}")
            except Exception as e:
                logger.warning(f"Не удалось загрузить состояние: {e}")

    def _save_state(self):
        """Сохраняет состояние."""
        try:
            with open(self.state_file, 'wb') as f:
                pickle.dump(self.crawler_status, f)
            logger.debug("Состояние сохранено")
        except Exception as e:
            logger.error(f"Ошибка сохранения состояния: {e}")

    def _update_crawler_state(self, source: str, status: Dict):
        """Обновляет состояние робота."""
        self.crawler_status[source] = {
            **status,
            'last_updated': datetime.now()
        }
        self._save_state()

    def _fetch_with_retry(self, url: str, method: str = 'GET',
                          data: Dict = None, params: Dict = None, timeout: int = None) -> Optional[requests.Response]:
        """Выполняет запрос с повторными попытками и ротацией заголовков."""
        if timeout is None:
            timeout = self.request_timeout

        for attempt in range(self.max_retries):
            try:
                headers = random.choice(self.headers_list)
                self.session.headers.update(headers)

                if method == 'GET':
                    response = self.session.get(url, params=params, timeout=timeout, allow_redirects=True)
                elif method == 'POST':
                    response = self.session.post(url, json=data, timeout=timeout)
                else:
                    raise ValueError(f"Неизвестный метод: {method}")

                if response.status_code == 200:
                    return response
                elif response.status_code in [403, 429]:
                    logger.warning(f"Доступ запрещен для {url}, статус: {response.status_code}")
                    time.sleep(30)
                    continue
                else:
                    logger.warning(f"Неудачный статус для {url}: {response.status_code}")

            except requests.exceptions.SSLError as e:
                logger.warning(f"SSL ошибка для {url} (попытка {attempt + 1}): {e}")
                time.sleep(5)
            except requests.exceptions.Timeout as e:
                logger.warning(f"Таймаут для {url} (попытка {attempt + 1}): {e}")
                time.sleep(10)
            except requests.exceptions.RequestException as e:
                logger.warning(f"Ошибка запроса для {url} (попытка {attempt + 1}): {e}")
                time.sleep(5)

            sleep_time = min(60, (2 ** attempt) + random.random() * 5)
            time.sleep(sleep_time)

        logger.error(f"Не удалось загрузить {url} после {self.max_retries} попыток")
        return None

    def _normalize_url(self, url: str, base_domain: str) -> str:
        """Нормализует URL."""
        try:
            parsed = urlparse(url)

            if not parsed.netloc:
                parsed = urlparse(urljoin(f"https://{base_domain}", url))

            clean_url = urlunparse((
                parsed.scheme or 'https',
                parsed.netloc,
                parsed.path.rstrip('/'),
                '',
                '',
                ''
            ))

            return clean_url
        except Exception as e:
            logger.debug(f"Ошибка нормализации URL {url}: {e}")
            return url

    def _is_valid_url(self, url: str, base_domain: str) -> bool:
        """Проверяет, является ли URL допустимым для сканирования."""
        try:
            parsed = urlparse(url)

            if not parsed.netloc:
                return True

            if base_domain not in parsed.netloc:
                return False

            invalid_extensions = ['.jpg', '.jpeg', '.png', '.gif', '.pdf',
                                  '.zip', '.mp4', '.mp3', '.css', '.js']
            if any(parsed.path.lower().endswith(ext) for ext in invalid_extensions):
                return False

            if parsed.scheme not in ['http', 'https']:
                return False

            return True

        except Exception:
            return False

    def _is_article_url(self, url: str, article_patterns: List[str]) -> bool:
        """Проверяет, является ли URL статьей."""
        url_lower = url.lower()

        if any(pattern in url_lower for pattern in article_patterns):
            return True

        article_indicators = ['/article/', '/story/', '/news/', '/blog/', '/post/',
                              '/fashion/', '/style/', '/trends/', '/collection/',
                              '/runway/', '/designer/', '/couture/', '/lookbook/']

        return any(indicator in url_lower for indicator in article_indicators)

    def _extract_links(self, html: str, base_url: str, base_domain: str) -> List[str]:
        """Извлекает ссылки из HTML."""
        soup = BeautifulSoup(html, 'html.parser')
        links = []

        for link in soup.find_all('a', href=True):
            href = link['href'].strip()

            if not href or href.startswith('#'):
                continue

            try:
                absolute_url = urljoin(base_url, href)
                normalized_url = self._normalize_url(absolute_url, base_domain)

                if self._is_valid_url(normalized_url, base_domain):
                    links.append(normalized_url)
            except Exception as e:
                logger.debug(f"Ошибка обработки ссылки {href}: {e}")
                continue

        return list(set(links))

    def _extract_content(self, url: str) -> Tuple[Optional[str], Optional[str]]:
        """Извлекает контент статьи (общий метод)."""
        # Если это URL Википедии, используем API
        if 'wikipedia.org/wiki/' in url:
            try:
                title = url.split('/wiki/')[-1].replace('_', ' ')
                page = self.wiki_api.page(title)

                if page.exists():
                    return page.title, page.text
            except Exception as e:
                logger.debug(f"Не удалось получить статью через API: {e}")
        try:
            response = self._fetch_with_retry(url)
            if not response:
                return None, None

            content_type = response.headers.get('Content-Type', '').lower()
            if 'text/html' not in content_type:
                logger.debug(f"Неподдерживаемый тип контента: {content_type}")
                return None, None

            soup = BeautifulSoup(response.content, 'html.parser')

            # Извлекаем заголовок
            title = None
            title_selectors = [
                'h1', 'h1.article-title', 'h1.post-title', 'h1.entry-title',
                'title', 'meta[property="og:title"]', 'meta[name="twitter:title"]'
            ]

            for selector in title_selectors:
                element = soup.select_one(selector)
                if element:
                    if selector.startswith('meta'):
                        title = element.get('content', '')
                    else:
                        title = element.get_text(strip=True)
                    if title and len(title) > 10:
                        break

            for tag in soup(['script', 'style', 'iframe', 'nav', 'footer',
                             'aside', 'header', 'form', 'button', '.ad', '.social-share',
                             '.newsletter', '.comments', '.related-posts']):
                tag.decompose()

            # Ищем основной контент
            content = None
            content_selectors = [
                'article', 'main', '[role="main"]',
                '.article-content', '.post-content', '.entry-content',
                '.article-body', '.story-body', '.content-body',
                '[class*="article"]', '[class*="content"]',
                '[class*="post"]', '[class*="story"]'
            ]

            for selector in content_selectors:
                elements = soup.select(selector)
                if elements:
                    # Берем самый большой элемент
                    largest_element = max(elements, key=lambda x: len(x.get_text()))
                    content = largest_element.get_text(separator=' ', strip=True)
                    content = re.sub(r'\s+', ' ', content)

                    if content and len(content.split()) >= 100:
                        break

            # Резервный вариант: собираем все параграфы
            if not content or len(content.split()) < 100:
                paragraphs = soup.find_all('p')
                if paragraphs:
                    text_parts = []
                    for p in paragraphs:
                        text = p.get_text(strip=True)
                        if len(text.split()) > 10:  # Пропускаем короткие параграфы
                            text_parts.append(text)

                    if text_parts:
                        content = ' '.join(text_parts[:20])  # Ограничиваем количество
                        content = re.sub(r'\s+', ' ', content)

            if not content or len(content.split()) < 200:
                logger.debug(f"Слишком короткий контент: {len(content.split()) if content else 0} слов")
                return None, None

            # Проверяем тематику
            content_lower = content.lower()
            fashion_terms = sum(1 for keyword in self.fashion_keywords
                                if keyword.lower() in content_lower)

            if fashion_terms < 3:
                logger.debug(f"Недостаточно терминов моды: {fashion_terms}")
                return None, None

            return title, content.strip()

        except Exception as e:
            logger.error(f"Ошибка извлечения контента из {url}: {e}")
            return None, None

    def _process_article(self, url: str, source_name: str) -> bool:
        """Обрабатывает статью."""
        try:
            # Проверяем, не обрабатывали ли уже
            existing = self.documents.find_one({'url': url})
            if existing:
                last_crawled = existing.get('last_crawled')
                if last_crawled and isinstance(last_crawled, datetime):
                    if (datetime.now() - last_crawled).days < self.recheck_days:
                        logger.debug(f"Статья недавно проверялась: {url}")
                        return False

            title, content = self._extract_content(url)
            if not content:
                return False

            # Вычисляем хэш
            content_hash = hashlib.md5(content.encode('utf-8')).hexdigest()

            # Если статья уже существует
            if existing:
                if existing.get('content_hash') == content_hash:
                    # Обновляем только дату
                    self.documents.update_one(
                        {'_id': existing['_id']},
                        {'$set': {'last_crawled': datetime.now()}}
                    )
                    return True
                else:
                    # Обновляем контент
                    update_data = {
                        'title': title or existing.get('title', ''),
                        'content': content,
                        'content_hash': content_hash,
                        'last_crawled': datetime.now(),
                        'last_modified': datetime.now(),
                        'word_count': len(content.split()),
                        'update_count': existing.get('update_count', 0) + 1
                    }
                    self.documents.update_one(
                        {'_id': existing['_id']},
                        {'$set': update_data}
                    )
                    logger.info(f"Обновлена статья: {url}")
                    return True
            else:
                # Новая статья
                doc = {
                    'url': url,
                    'title': title or self._extract_title_from_url(url),
                    'content': content,
                    'content_hash': content_hash,
                    'source': source_name,
                    'word_count': len(content.split()),
                    'first_crawled': datetime.now(),
                    'last_crawled': datetime.now(),
                    'update_count': 0
                }
                self.documents.insert_one(doc)
                logger.info(f"Добавлена новая статья: {url} ({doc['word_count']} слов)")
                return True

        except Exception as e:
            logger.error(f"Ошибка обработки статьи {url}: {e}")
            return False

    def _extract_title_from_url(self, url: str) -> str:
        """Извлекает заголовок из URL."""
        try:
            parsed = urlparse(url)
            path = parsed.path.strip('/')

            if path:
                parts = path.split('/')
                if parts:
                    last_part = parts[-1]
                    title = last_part.replace('-', ' ').replace('_', ' ')
                    if len(title.split()) > 1:
                        return title.title()

            return f"Статья с {parsed.netloc}"
        except:
            return "Статья о моде"

    def _recursive_crawl_site(self, source_config: Dict, max_depth: int = 3) -> int:
        """Рекурсивно сканирует сайт."""
        source_name = source_config['name']
        base_domain = urlparse(source_config['base_url']).netloc
        article_patterns = source_config.get('article_patterns', [])
        start_urls = source_config.get('start_urls', [source_config['base_url']])

        logger.info(f"Рекурсивное сканирование {source_name} (глубина: {max_depth})")

        visited = set()
        queue = deque()
        articles_found = 0

        for url in start_urls:
            queue.append((url, 0))  # (url, depth)

        while queue and articles_found < 1000:
            current_url, depth = queue.popleft()

            # Пропускаем если уже посещали или слишком глубокая рекурсия
            if current_url in visited or depth > max_depth:
                continue

            visited.add(current_url)
            logger.debug(f"Сканирование: {current_url} (глубина: {depth})")

            try:
                response = self._fetch_with_retry(current_url, timeout=60)
                if not response:
                    continue

                if self._is_article_url(current_url, article_patterns):
                    if self._process_article(current_url, source_name):
                        articles_found += 1

                if depth < max_depth:
                    links = self._extract_links(response.text, current_url, base_domain)

                    for link in links:
                        if link not in visited:
                            if self._is_article_url(link, article_patterns):
                                queue.appendleft((link, depth + 1))
                            else:
                                queue.append((link, depth + 1))

                time.sleep(self.crawl_delay + random.random() * 2)

                if articles_found % 10 == 0:
                    logger.info(f"Найдено {articles_found} статей на {source_name}")

                if articles_found % 20 == 0:
                    self._update_crawler_state(source_name, {
                        'last_url': current_url,
                        'articles_found': articles_found,
                        'urls_visited': len(visited),
                        'depth': depth
                    })

            except Exception as e:
                logger.error(f"Ошибка при сканировании {current_url}: {e}")
                continue

        logger.info(f"Завершено сканирование {source_name}: {articles_found} статей")
        self._update_crawler_state(source_name, {
            'last_url': None,
            'articles_found': articles_found,
            'urls_visited': len(visited),
            'depth': max_depth,
            'completed': True
        })

        return articles_found

    def _recursive_crawl_wikipedia(self, max_docs: int = 2000, max_depth: int = 3) -> int:
        """Рекурсивно сканирует Википедию через API."""
        logger.info(f"Рекурсивное сканирование Википедии через API (глубина: {max_depth})")

        root_categories = [
            'Category:Fashion',
            'Category:Clothing',
            'Category:Textiles',
            'Category:Fashion_designers'
        ]

        articles_found = 0
        visited_categories = set()
        visited_articles = set()

        def crawl_category(category_title: str, depth: int):
            nonlocal articles_found

            if depth > max_depth or articles_found >= max_docs:
                return

            category_key = (category_title, depth)
            if category_key in visited_categories:
                return

            visited_categories.add(category_key)
            logger.info(f"Обработка категории: {category_title} (глубина: {depth})")

            try:
                category = self.wiki_api.page(category_title)

                if not category.exists():
                    logger.warning(f"Категория не найдена: {category_title}")
                    return

                members = category.categorymembers

                for member in members.values():
                    if articles_found >= max_docs:
                        return

                    # Обрабатываем статью
                    if member.ns == wikipediaapi.Namespace.MAIN:
                        if member.pageid not in visited_articles:
                            if self._process_wiki_article(member):
                                articles_found += 1
                                visited_articles.add(member.pageid)

                                if articles_found % 10 == 0:
                                    logger.info(f"Найдено {articles_found} статей Википедии")

                    # Рекурсивно обрабатываем подкатегории
                    elif (member.ns == wikipediaapi.Namespace.CATEGORY and
                          depth < max_depth and
                          'disambiguation' not in member.title.lower()):
                        crawl_category(member.title, depth + 1)

                    time.sleep(0.1)

            except Exception as e:
                logger.error(f"Ошибка при обработке категории {category_title}: {e}")

        # Запускаем рекурсивный обход
        for category in root_categories:
            if articles_found >= max_docs:
                break
            crawl_category(category, 0)

        logger.info(f"Завершено сканирование Википедии: {articles_found} статей")
        return articles_found

    def _process_wiki_article(self, page) -> bool:
        """Обрабатывает статью Википедии через API."""
        try:
            url = f"https://en.wikipedia.org/wiki/{page.title.replace(' ', '_')}"

            # Проверяем, не обрабатывали ли уже
            existing = self.documents.find_one({'url': url})
            if existing:
                last_crawled = existing.get('last_crawled')
                if last_crawled and isinstance(last_crawled, datetime):
                    if (datetime.now() - last_crawled).days < self.recheck_days:
                        return False

            # Получаем контент через API
            if not page.exists():
                return False

            content = page.text
            if not content or len(content.split()) < 200:
                logger.debug(f"Слишком короткий контент: {len(content.split()) if content else 0} слов")
                return False

            # Проверяем тематику
            content_lower = content.lower()
            fashion_terms = sum(1 for keyword in self.fashion_keywords
                                if keyword.lower() in content_lower)

            if fashion_terms < 3:
                logger.debug(f"Недостаточно терминов моды: {fashion_terms}")
                return False

            content_hash = hashlib.md5(content.encode('utf-8')).hexdigest()

            if existing:
                update_data = {
                    'title': page.title,
                    'content': content,
                    'content_hash': content_hash,
                    'last_crawled': datetime.now(),
                    'last_modified': datetime.now(),
                    'word_count': len(content.split()),
                    'update_count': existing.get('update_count', 0) + 1
                }
                self.documents.update_one(
                    {'_id': existing['_id']},
                    {'$set': update_data}
                )
                logger.debug(f"Обновлена статья Википедии: {page.title}")
            else:
                doc = {
                    'url': url,
                    'title': page.title,
                    'content': content,
                    'content_hash': content_hash,
                    'source': 'Wikipedia',
                    'word_count': len(content.split()),
                    'first_crawled': datetime.now(),
                    'last_crawled': datetime.now(),
                    'update_count': 0,
                    'pageid': page.pageid  # Сохраняем ID страницы
                }
                self.documents.insert_one(doc)
                logger.info(f"Добавлена статья Википедии: {page.title} ({doc['word_count']} слов)")

            return True

        except Exception as e:
            logger.error(
                f"Ошибка обработки статьи Википедии {page.title if hasattr(page, 'title') else 'unknown'}: {e}")
            return False

    def crawl_all_sites(self):
        """Сканирует все сайты."""
        total_articles = 0

        # Сортируем по приоритету
        sources_sorted = sorted(self.source_configs, key=lambda x: x.get('priority', 3))

        for source_config in sources_sorted:
            try:
                logger.info(f"Начинаем сканирование: {source_config['name']}")
                articles = self._recursive_crawl_site(source_config, max_depth=3)
                total_articles += articles
                logger.info(f"Завершено {source_config['name']}: {articles} статей")

                time.sleep(10)

            except KeyboardInterrupt:
                logger.info("Сканирование прервано пользователем")
                break
            except Exception as e:
                logger.error(f"Ошибка при сканировании {source_config['name']}: {e}")
                continue

        return total_articles

    def generate_statistics(self):
        """Генерирует статистику."""
        total_docs = self.documents.count_documents({})

        if total_docs == 0:
            logger.warning("Корпус пуст!")
            return

        # Статистика по источникам
        pipeline = [
            {'$group': {
                '_id': '$source',
                'count': {'$sum': 1},
                'total_words': {'$sum': '$word_count'},
                'avg_words': {'$avg': '$word_count'},
                'min_words': {'$min': '$word_count'},
                'max_words': {'$max': '$word_count'}
            }},
            {'$sort': {'count': -1}}
        ]

        stats = list(self.documents.aggregate(pipeline))

        total_stats = {
            'total_documents': total_docs,
            'total_words': sum(s['total_words'] for s in stats),
            'avg_words_per_doc': sum(s['total_words'] for s in stats) / total_docs,
            'sources': stats,
            'generated_at': datetime.now().isoformat()
        }

        with open('corpus_statistics.json', 'w', encoding='utf-8') as f:
            json.dump(total_stats, f, ensure_ascii=False, indent=2)

        self._print_report(total_stats)
        return total_stats

    def _print_report(self, stats):
        """Печатает отчет."""
        report = f"""
{'=' * 80}
ОТЧЕТ ПОИСКОВОГО РОБОТА
{'=' * 80}

ОБЩАЯ СТАТИСТИКА:
• Всего документов: {stats['total_documents']:,}
• Всего слов: {stats['total_words']:,}
• Средний размер документа: {stats['avg_words_per_doc']:,.0f} слов

РАСПРЕДЕЛЕНИЕ ПО ИСТОЧНИКАМ:
"""

        for source in stats['sources']:
            report += f"• {source['_id']}: {source['count']:,} документов\n"
            report += f"  Средний размер: {source['avg_words']:.0f} слов\n"

        report += f"\n{'=' * 80}\n"
        print(report)

        with open('corpus_report.txt', 'w', encoding='utf-8') as f:
            f.write(report)

    def cleanup(self):
        """Очистка."""
        self.session.close()
        logger.info("Краулер остановлен")


def main():
    """Основная функция."""
    print("""
    ╔══════════════════════════════════════════════════════════════╗
    ║  ПРОДВИНУТЫЙ РОБОТ С РЕКУРСИВНЫМ ОБХОДОМ (ГЛУБИНА 3)         ║
    ╚══════════════════════════════════════════════════════════════╝
    """)

    crawler = None

    try:
        crawler = AdvancedFashionCrawler()

        print("✓ Робот инициализирован")
        print("✓ SSL проверка отключена для проблемных сайтов")

        crawler.client.admin.command('ping')
        print("✓ MongoDB подключена")

        print("\n" + "=" * 60)
        print("Выберите режим:")
        print("1. Рекурсивный обход сайтов (глубина 3)")
        print("2. Рекурсивный обход Википедии (глубина 3)")
        print("3. Полный обход (сайты + Википедия)")
        print("4. Только статистика")

        choice = input("\nВаш выбор (1-4): ").strip()

        if choice == '1':
            print("\n" + "=" * 60)
            print("РЕКУРСИВНЫЙ ОБХОД САЙТОВ (ГЛУБИНА 3)")
            print("=" * 60)
            total = crawler.crawl_all_sites()
            print(f"\n✓ Найдено статей на сайтах: {total}")

        elif choice == '2':
            print("\n" + "=" * 60)
            print("РЕКУРСИВНЫЙ ОБХОД ВИКИПЕДИИ (ГЛУБИНА 3)")
            print("=" * 60)
            total = crawler._recursive_crawl_wikipedia(max_docs=30000, max_depth=3)
            print(f"\n✓ Найдено статей в Википедии: {total}")

        elif choice == '3':
            print("\n" + "=" * 60)
            print("ПОЛНЫЙ ОБХОД")
            print("=" * 60)
            # Википедия
            wiki_total = crawler._recursive_crawl_wikipedia(max_docs=30000, max_depth=3)
            print(f"✓ Википедия: {wiki_total} статей")

            # Сайты
            print("\n" + "-" * 60)
            print("НАЧИНАЕМ ОБХОД САЙТОВ")
            print("-" * 60)
            sites_total = crawler.crawl_all_sites()
            print(f"✓ Сайты: {sites_total} статей")

            total = wiki_total + sites_total
            print(f"\n✓ Всего: {total} статей")

        elif choice == '4':
            print("\n" + "=" * 60)
            print("СТАТИСТИКА")
            print("=" * 60)
            stats = crawler.generate_statistics()
            if stats:
                print(f"\n✓ Статистика сохранена")
            else:
                print("\n⚠️  Корпус пуст")
        else:
            print("\n❌ Неверный выбор")
            return

        # Финальная статистика
        if choice in ['1', '2', '3']:
            print("\n" + "=" * 60)
            print("ФИНАЛЬНАЯ СТАТИСТИКА")
            print("=" * 60)
            crawler.generate_statistics()

        print("\n" + "=" * 80)
        print("✅ РАБОТА ЗАВЕРШЕНА!")
        print("=" * 80)
        print("\nСостояние сохранено. Можно перезапустить для продолжения.")

    except KeyboardInterrupt:
        print("\n\n⚠️  ПРЕРВАНО ПОЛЬЗОВАТЕЛЕМ")
        print("Состояние сохранено.")
    except Exception as e:
        print(f"\n❌ ОШИБКА: {e}")
        import traceback
        traceback.print_exc()
    finally:
        if crawler:
            crawler.cleanup()


if __name__ == "__main__":
    main()