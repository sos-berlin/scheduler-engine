jQuery(document).ready(function($) {
    $(".clickable").click(function() {
      window.location = $(this).data("href");
    });
    $("body").bind('keypress', function keyPressed(e) {
        if ((e.keyCode || e.which) == 13) {
            window.location.href = window.location.href;
        }
    });
});
