/* --- 전역 변수 및 요소 가져오기 --- */
const galleryContainer = document.getElementById('galleryContainer');
const backdrop = document.getElementById('backdrop');
const colsInput = document.getElementById('colsInput');
const rowsInput = document.getElementById('rowsInput');
const imageUrlsInput = document.getElementById('imageUrls');
const updateGridBtn = document.getElementById('updateGridBtn');

// --- 추가된 UI 요소 ---
const openSettingsBtn = document.getElementById('openSettingsBtn');
const closeSettingsBtn = document.getElementById('closeSettingsBtn');
const settingsModal = document.getElementById('settingsModal');

// --- 갤러리 기능 함수화 ---

/**
 * 갤러리 아이템에 클릭 시 확대/축소되는 이벤트 리스너를 설정하는 함수
 */
function setupGalleryEventListeners() {
    const galleryItems = document.querySelectorAll('.gallery-item');
    galleryItems.forEach(item => {
        const newItem = item.cloneNode(true);
        item.parentNode.replaceChild(newItem, item);

        newItem.addEventListener('click', () => {
            if (newItem.classList.contains('empty')) return;

            const currentFocused = document.querySelector('.gallery-item.focused');
            if (currentFocused && currentFocused !== newItem) {
                currentFocused.classList.remove('focused');
            }
            newItem.classList.toggle('focused');
            
            backdrop.style.display = newItem.classList.contains('focused') ? 'block' : 'none';
        });
    });
}

/**
 * 입력된 설정에 따라 그리드를 새로 생성하는 함수
 */
function updateGrid() {
    const cols = parseInt(colsInput.value, 10);
    const rows = parseInt(rowsInput.value, 10);
    const imageUrls = imageUrlsInput.value.split('\n').filter(url => url.trim() !== '');
    const totalCells = cols * rows;

    galleryContainer.style.gridTemplateColumns = `repeat(${cols}, 1fr)`;
    galleryContainer.innerHTML = '';

    for (let i = 0; i < totalCells; i++) {
        const item = document.createElement('div');
        item.classList.add('gallery-item');
        const url = imageUrls[i] ? imageUrls[i].trim() : '';

        if (url) {
            // [핵심 변경] RTSP 주소인지 감지
            if (url.startsWith('rtsp://')) {
                item.classList.add('stream-mode');

                // iframe 생성
                const iframe = document.createElement('iframe');
                
                // Nginx 프록시 경로(/stream/)를 통해 go2rtc 플레이어 호출
                // src 파라미터에 RTSP 주소를 넣고, mode를 webrtc로 설정
                iframe.src = `/stream/stream.html?src=${encodeURIComponent(url)}&mode=webrtc`;
                
                // 스타일: 꽉 차게, 테두리 없이
                iframe.style.width = '100%';
                iframe.style.height = '100%';
                iframe.style.border = 'none';
                
                // 중요: 클릭 이벤트를 상위 div가 받도록 설정 (설정 모달 띄우기 위해)
                iframe.style.pointerEvents = 'none'; 
                
                item.appendChild(iframe);
                item.dataset.type = 'stream'; // 스트림 타입 표시
            } else {
                // 기존 이미지 처리 로직
                const img = document.createElement('img');
                img.src = url;
                img.alt = `이미지 ${i + 1}`;
                item.appendChild(img);
                item.dataset.type = 'image';
            }
        } else {
            item.classList.add('empty');
        }
        galleryContainer.appendChild(item);
    }

    setupGalleryEventListeners();
}

// --- 이벤트 리스너 설정 ---

// 배경 클릭 시 열려있는 창 닫기
backdrop.addEventListener('click', () => {
    const focusedItem = document.querySelector('.gallery-item.focused');
    if (focusedItem) {
        focusedItem.classList.remove('focused');
        backdrop.style.display = 'none';
    }
    
    if (settingsModal.style.display === 'block') {
        settingsModal.style.display = 'none';
        backdrop.style.display = 'none';
    }
});

// "그리드 업데이트" 버튼 클릭 이벤트
updateGridBtn.addEventListener('click', () => {
    updateGrid();
    settingsModal.style.display = 'none';
    backdrop.style.display = 'none';
});

// '설정' 버튼 클릭 시 모달 열기
openSettingsBtn.addEventListener('click', () => {
    settingsModal.style.display = 'block';
    backdrop.style.display = 'block';
});

// 모달의 '닫기' 버튼 클릭 시 모달 닫기
closeSettingsBtn.addEventListener('click', () => {
    settingsModal.style.display = 'none';
    backdrop.style.display = 'none';
});

// --- 페이지 로드 시 초기 그리드 생성 ---
document.addEventListener('DOMContentLoaded', updateGrid);


// --- 기존 로그 및 알림 관련 코드 ---
let counter = 0;
const counterElement = document.getElementById('counter');
const textBox = document.getElementById('textBox');
let last_alarm = "";

function updateCloseAllButtonVisibility() {
    const container = document.getElementById('notification-container');
    const notifications = container.querySelectorAll('.toast-notification');
    let closeAllBtn = document.getElementById('close-all-btn');

    if (notifications.length >= 2) {
        if (!closeAllBtn) {
            closeAllBtn = document.createElement('button');
            closeAllBtn.id = 'close-all-btn';
            closeAllBtn.innerText = '알림 모두 닫기';
            
            closeAllBtn.onclick = () => {
                container.querySelectorAll('.toast-notification').forEach(n => {
                    n.classList.remove('show');
                    n.addEventListener('transitionend', () => n.remove());
                });
                closeAllBtn.classList.remove('show');
                closeAllBtn.addEventListener('transitionend', () => closeAllBtn.remove());
            };
        }
        container.appendChild(closeAllBtn);
        setTimeout(() => closeAllBtn.classList.add('show'), 10);
    } else if (closeAllBtn) {
        closeAllBtn.classList.remove('show');
        closeAllBtn.addEventListener('transitionend', () => closeAllBtn.remove());
    }
}

function createNotification(message) {
    const container = document.getElementById('notification-container');
    const notification = document.createElement('div');
    notification.className = 'toast-notification';
    
    // --- ✨ 수정된 부분: 정규식 및 헤더 생성 로직 변경 ---

    // 정규식을 사용해 '시간', 'Thread 번호', '본문'을 각각 별도의 그룹으로 추출합니다.
    // 예: "[2025-10-01 17:02:15 | Thread 1 | ALARM] | [ 1번 채널 ]..."
    const regex = /^\[(.*?)\s*\|\s*Thread\s*(\d+).*?\]\s*\|\s*(.*)$/;
    const match = message.match(regex);

    let headerText = '';
    let bodyText = '';

    if (match && match.length === 4) {
        // 정규식 매칭 성공
        const timestamp = match[1].trim(); // 그룹 1: 시간 (예: "2025-10-01 17:02:15")
        const channelNum = match[2].trim();  // 그룹 2: Thread 번호 (예: "1")
        
        // 추출한 정보로 원하는 헤더 형식을 새로 만듭니다.
        headerText = `${timestamp} | Channel ${channelNum}`;
        
        bodyText = match[3].trim();      // 그룹 3: "|" 뒷부분 전체
    } else {
        // 정규식 매칭 실패 시: 전체 메시지를 본문에 표시 (호환성 유지)
        bodyText = message;
    }

    // 최종적으로 완성된 헤더와 본문으로 HTML 구조를 생성
    notification.innerHTML = `
        <div class="toast-content">
            ${headerText ? `<div class="toast-header">${headerText}</div>` : ''}
            <div class="toast-body">${bodyText}</div>
        </div>
        <button class="close-btn">&times;</button>
    `;
    
    container.appendChild(notification);
    
    setTimeout(() => {
        notification.classList.add('show');
    }, 10);

    notification.querySelector('.close-btn').onclick = () => {
        notification.classList.remove('show');
        notification.addEventListener('transitionend', () => {
            notification.remove();
            updateCloseAllButtonVisibility();
        });
    };

    container.scrollTop = container.scrollHeight;
    updateCloseAllButtonVisibility();
}

// 이미지 실시간 업데이트 로직
setInterval(() => {
    const timestamp = new Date().getTime();
    // .gallery-item 내부의 순수 'img' 태그만 선택
    const allImages = document.querySelectorAll('.gallery-item:not(.empty) > img');
    
    allImages.forEach(img => {
        let originalSrc = img.src.split('?')[0];
        img.src = `${originalSrc}?t=${timestamp}`;
    });
}, 200); // 200ms 주기는 이미지에만 적용됨

// 로그 업데이트 로직
setInterval(() => {
    fetch('./app.log', { cache: 'no-store' })
        .then(response => {
            if (!response.ok) {
                throw new Error('로그 파일을 찾을 수 없습니다.');
            }
            return response.text();
        })
        .then(text => {
            textBox.innerHTML = '';
            let lines = text.split('\n');
            if (lines.length > 50) {
                lines = lines.slice(lines.length - 50);
            }

            lines.forEach(line => {
                if (line.trim() !== '') {
                    const lineElement = document.createElement('div');
                    lineElement.className = 'text-line';
                    lineElement.textContent = line;
                    textBox.appendChild(lineElement);
                }
            });

            textBox.scrollTop = textBox.scrollHeight;

            const allAlarmsInLog = lines.filter(line => line.includes("ALARM"));
            if (allAlarmsInLog.length > 0) {
                const lastAlarmIndex = last_alarm ? allAlarmsInLog.lastIndexOf(last_alarm) : -1;
                const newAlarms = allAlarmsInLog.slice(lastAlarmIndex + 1);
                
                newAlarms.forEach(alarm => {
                    createNotification(alarm);
                });
                
                if (newAlarms.length > 0) {
                    last_alarm = newAlarms[newAlarms.length - 1];
                }
            }
        })
        .catch(error => {
            console.error('로그 파일 로딩 오류:', error);
        });
}, 3000);