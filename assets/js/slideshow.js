showAllSlides();

// Next/previous controls
function plusSlides(curObject, n) {
    let slideContainer = getParentSlideContainer(curObject);
    setSlideIndex(slideContainer, getSlideIndex(slideContainer) + n)
    showSlides(slideContainer, getSlideIndex(slideContainer));
}

// Thumbnail image controls
function currentSlide(curObject, n) {
    let slideContainer = getParentSlideContainer(curObject);
    setSlideIndex(slideContainer, n)
    showSlides(slideContainer, getSlideIndex(slideContainer));
}

function showAllSlides() {
  let slideContainers = document.getElementsByClassName('slideshow-container');
  for (let item of slideContainers) {
    setSlideIndex(item, 1)
    showSlides(item, 1);
  };
} 

function showSlides(slideContainer, n) {
    let slides = slideContainer.querySelectorAll('.slides');
    let dots = slideContainer.querySelectorAll('.slide-dot');
    if (n > slides.length) {
        setSlideIndex(slideContainer, 1)
    }
    if (n < 1) {
        setSlideIndex(slideContainer, slides.length)
    }
    for (let i = 0; i < slides.length; i++) {
        slides[i].style.display = 'none';
    }
    for (let i = 0; i < dots.length; i++) {
        dots[i].className = dots[i].className.replace(' slide-active', '');
    }
    slides[getSlideIndex(slideContainer) - 1].style.display = 'block';
    dots[getSlideIndex(slideContainer) - 1].className += ' slide-active';
  } 

function getParentSlideContainer(curObject) {
    while (!curObject.classList.contains('slideshow-container')) {
        curObject = curObject.parentElement   
    }
    return curObject;
}

function setSlideIndex(curObject, idx) {
    curObject.setAttribute('data-slide-index', idx)
}

function getSlideIndex(curObject) {
    return parseInt(curObject.getAttribute('data-slide-index'))
}